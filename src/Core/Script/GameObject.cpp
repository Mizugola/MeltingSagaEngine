#include <Scene/Scene.hpp>
#include <Script/Exceptions.hpp>
#include <Script/GameObject.hpp>
#include <Script/ViliLuaBridge.hpp>
#include <System/Loaders.hpp>
#include <Triggers/Trigger.hpp>
#include <Triggers/TriggerManager.hpp>
#include <Utils/StringUtils.hpp>

namespace obe::Script
{
    sol::table GameObject::access() const
    {
        if (m_hasScriptEngine)
            return m_environment["Object"].get<sol::table>();
        throw Exceptions::NoSuchComponent("Script", m_type, m_id, EXC_INFO);
    }

    sol::function GameObject::getConstructor() const
    {
        if (m_hasScriptEngine)
            return m_environment["ObjectInit"].get<sol::function>();
        throw Exceptions::NoSuchComponent("Script", m_type, m_id, EXC_INFO);
    }

    vili::node GameObjectDatabase::allDefinitions;
    vili::node GameObjectDatabase::allRequires;
    vili::node& GameObjectDatabase::GetRequirementsForGameObject(const std::string& type)
    {
        if (!allRequires.root().contains(type))
        {
            vili::ViliParser getGameObjectFile;
            System::Path("Data/GameObjects/")
                .add(type)
                .add(type + ".obj.vili")
                .load(System::Loaders::dataLoader, getGameObjectFile);
            if (getGameObjectFile->contains("Requires"))
            {
                vili::ComplexNode& requiresData
                    = getGameObjectFile.at<vili::ComplexNode>("Requires");
                getGameObjectFile->extractElement(
                    &getGameObjectFile.at<vili::ComplexNode>("Requires"));
                requiresData.setId(type);
                allRequires->pushComplexNode(&requiresData);
                return &requiresData;
            }
            return nullptr;
        }
        return &allRequires.at(type);
    }

    vili::ComplexNode* GameObjectDatabase::GetDefinitionForGameObject(
        const std::string& type)
    {
        if (!allDefinitions.root().contains(type))
        {
            vili::ViliParser getGameObjectFile;
            auto result = System::Path("Data/GameObjects/")
                              .add(type)
                              .add(type + ".obj.vili")
                              .load(System::Loaders::dataLoader, getGameObjectFile);
            if (!result)
                throw Exceptions::ObjectDefinitionNotFound(type, EXC_INFO);
            if (getGameObjectFile->contains(type))
            {
                vili::ComplexNode& definitionData
                    = getGameObjectFile.at<vili::ComplexNode>(type);
                getGameObjectFile->extractElement(
                    &getGameObjectFile.at<vili::ComplexNode>(type));
                definitionData.setId(type);
                allDefinitions->pushComplexNode(&definitionData);
                return &definitionData;
            }
            throw Exceptions::ObjectDefinitionBlockNotFound(type, EXC_INFO);
        }
        return &allDefinitions.at(type);
    }

    void GameObjectDatabase::ApplyRequirements(
        sol::environment environment, vili::ComplexNode& requires)
    {
        for (vili::Node* currentRequirement : requires.getAll())
        {
            const sol::table requireTable
                = environment["LuaCore"]["ObjectInitInjectionTable"].get<sol::table>();
            DataBridge::dataToLua(requireTable, currentRequirement);
        }
    }

    void GameObjectDatabase::Clear()
    {
        allDefinitions->clear();
        allRequires->clear();
    }

    // GameObject
    GameObject::GameObject(Triggers::TriggerManager& triggers, sol::state_view lua,
        const std::string& type, const std::string& id)
        : Identifiable(id)
        , m_triggers(triggers)
        , m_lua(lua)
    {
        m_type = type;
    }

    void GameObject::initialize()
    {
        if (!m_active)
        {
            Debug::Log->debug(
                "<GameObject> Initializing GameObject '{0}' ({1})", m_id, m_type);
            m_active = true;
            if (m_hasScriptEngine)
            {
                m_environment["__OBJECT_INIT"] = true;
                t_local->trigger("Init");
            }
        }
        else
            Debug::Log->warn("<GameObject> GameObject '{0}' ({1}) has already "
                             "been initialized",
                m_id, m_type);
    }

    GameObject::~GameObject()
    {
        Debug::Log->debug("<GameObject> Deleting GameObject '{0}' ({1})", m_id, m_type);
        if (m_hasScriptEngine)
        {

            t_local.reset();
            m_triggers.removeNamespace(m_privateKey);
        }
    }

    void GameObject::sendInitArgFromLua(
        const std::string& argName, sol::object value) const
    {
        Debug::Log->debug("<GameObject> Sending Local.Init argument {0} to "
                          "GameObject {1} ({2}) (From Lua)",
            argName, m_id, m_type);
        t_local->pushParameterFromLua("Init", argName, value);
    }

    void GameObject::registerTrigger(
        std::weak_ptr<Triggers::Trigger> trg, const std::string& callbackName)
    {
        m_registeredTriggers.emplace_back(trg, callbackName);
    }

    void GameObject::loadGameObject(
        Scene::Scene& scene, vili::ComplexNode& obj, Engine::ResourceManager* resources)
    {
        Debug::Log->debug("<GameObject> Loading GameObject '{0}' ({1})", m_id, m_type);
        // Script
        if (obj.contains(vili::NodeType::DataNode, "permanent"))
        {
            m_permanent = obj.getDataNode("permanent").get<bool>();
        }
        if (obj.contains(vili::NodeType::ComplexNode, "Script"))
        {
            m_hasScriptEngine = true;
            m_environment = sol::environment(m_lua, sol::create, m_lua.globals());
            m_privateKey = Utils::String::getRandomKey(Utils::String::Alphabet, 1)
                + Utils::String::getRandomKey(
                    Utils::String::Alphabet + Utils::String::Numbers, 11);
            m_triggers.createNamespace(m_privateKey);
            t_local = m_triggers.createTriggerGroup(m_privateKey, "Local");

            m_environment["This"] = this;

            t_local->add("Init").add("Delete");

            m_environment["__OBJECT_TYPE"] = m_type;
            m_environment["__OBJECT_ID"] = m_id;
            m_environment["__OBJECT_INIT"] = false;
            m_environment["Private"] = m_privateKey;

            m_lua.safe_script_file("Lib/Internal/ObjectInit.lua"_fs, m_environment);

            auto loadSource = [&](const std::string& path) {
                const std::string fullPath = System::Path(path).find();
                if (fullPath.empty())
                {
                    throw Exceptions::ScriptFileNotFound(m_type, m_id, path, EXC_INFO);
                }
                m_lua.safe_script_file(fullPath, m_environment);
            };
            if (obj.at("Script").contains("source"))
            {
                const vili::Node* sourceNode = obj.at("Script").get("source");
                if (sourceNode->getType() == vili::NodeType::DataNode
                    && obj.at("Script").getDataNode("source").getDataType()
                        == vili::DataType::String)
                {
                    loadSource(obj.at("Script").getDataNode("source").get<std::string>());
                }
                else
                {
                    std::string nodeType
                        = vili::Types::nodeTypeToString(sourceNode->getType());
                    if (sourceNode->getType() == vili::NodeType::DataNode)
                        nodeType += "::"
                            + vili::Types::dataTypeToString(
                                obj.at("Script").getDataNode("source").getDataType());
                    throw Exceptions::WrongSourceAttributeType(
                        m_type, "source", "DataNode::String", nodeType, EXC_INFO);
                }
            }
            else if (obj.at("Script").contains("sources"))
            {
                const vili::Node* sourceNode = obj.at("Script").get("sources");
                if (sourceNode->getType() == vili::NodeType::ArrayNode)
                {
                    const std::size_t scriptListSize
                        = obj.at("Script").getArrayNode("sources").size();
                    for (std::size_t i = 0; i < scriptListSize; i++)
                    {
                        loadSource(obj.at("Script")
                                       .getArrayNode("sources")
                                       .get(i)
                                       .get<std::string>());
                    }
                }
                else
                {
                    const std::string nodeType
                        = vili::Types::nodeTypeToString(sourceNode->getType());
                    throw Exceptions::WrongSourceAttributeType(
                        m_type, "sources", "ArrayNode", nodeType, EXC_INFO);
                }
            }
        }
        // Sprite
        if (obj.contains(vili::NodeType::ComplexNode, "Sprite"))
        {
            m_sprite = &scene.createSprite(m_id, false);
            m_objectNode.addChild(*m_sprite);
            m_sprite->load(obj.at("Sprite"));
            if (m_hasScriptEngine)
                m_environment["Object"]["Sprite"] = m_sprite;
            scene.reorganizeLayers();
        }
        if (obj.contains(vili::NodeType::ComplexNode, "Animator"))
        {
            m_animator = std::make_unique<Animation::Animator>();
            const std::string animatorPath
                = obj.at("Animator").getDataNode("path").get<std::string>();
            if (m_sprite)
                m_animator->setTarget(*m_sprite);
            if (animatorPath != "")
            {
                m_animator->load(System::Path(animatorPath), resources);
            }
            if (obj.at("Animator").contains(vili::NodeType::DataNode, "default"))
            {
                m_animator->setKey(
                    obj.at("Animator").getDataNode("default").get<std::string>());
            }
            if (m_hasScriptEngine)
                m_environment["Object"]["Animation"] = m_animator.get();
        }
        // Collider
        if (obj.contains(vili::NodeType::ComplexNode, "Collider"))
        {
            m_collider = &scene.createCollider(m_id, false);
            m_objectNode.addChild(*m_collider);
            m_collider->load(obj.at("Collider"));

            if (m_hasScriptEngine)
                m_environment["Object"]["Collider"] = m_collider;
        }
    }

    void GameObject::update()
    {
        if (m_canUpdate)
        {
            if (m_active)
            {
                if (m_animator)
                {
                    if (m_animator->getKey() != "")
                        m_animator->update();
                    if (m_sprite)
                    {
                        m_sprite->setTexture(m_animator->getTexture());
                    }
                }
            }
            else
            {
                this->initialize();
            }
        }
    }

    std::string GameObject::getType() const
    {
        return m_type;
    }

    bool GameObject::doesHaveAnimator() const
    {
        return static_cast<bool>(m_animator);
    }

    bool GameObject::doesHaveCollider() const
    {
        return static_cast<bool>(m_collider);
    }

    bool GameObject::doesHaveSprite() const
    {
        return static_cast<bool>(m_sprite);
    }

    bool GameObject::doesHaveScriptEngine() const
    {
        return m_hasScriptEngine;
    }

    bool GameObject::getUpdateState() const
    {
        return m_canUpdate;
    }

    void GameObject::setUpdateState(bool state)
    {
        m_canUpdate = state;
    }

    Graphics::Sprite& GameObject::getSprite() const
    {
        if (m_sprite)
            return *m_sprite;
        throw Exceptions::NoSuchComponent("Sprite", m_type, m_id, EXC_INFO);
    }

    Scene::SceneNode& GameObject::getSceneNode()
    {
        return m_objectNode;
    }

    Collision::PolygonalCollider& GameObject::getCollider() const
    {
        if (m_collider)
            return *m_collider;
        throw Exceptions::NoSuchComponent("Collider", m_type, m_id, EXC_INFO);
    }

    Animation::Animator& GameObject::getAnimator() const
    {
        if (m_animator)
            return *m_animator.get();
        throw Exceptions::NoSuchComponent("Animator", m_type, m_id, EXC_INFO);
    }

    void GameObject::useTrigger(const std::string& trNsp, const std::string& trGrp,
        const std::string& trName, const std::string& callAlias)
    {
        if (trName == "*")
        {
            std::vector<std::string> allTrg
                = m_triggers.getAllTriggersNameFromTriggerGroup(trNsp, trGrp);
            for (const std::string& triggerName : allTrg)
            {
                this->useTrigger(trNsp, trGrp, triggerName,
                    (Utils::String::occurencesInString(callAlias, "*")
                            ? Utils::String::replace(callAlias, "*", triggerName)
                            : ""));
            }
        }
        else
        {
            bool triggerNotFound = true;
            for (auto& triggerPair : m_registeredTriggers)
            {
                if (triggerPair.first.lock()
                    == m_triggers.getTrigger(trNsp, trGrp, trName).lock())
                {
                    triggerNotFound = false;
                }
            }
            if (triggerNotFound)
            {
                const std::string callbackName = (callAlias.empty())
                    ? trNsp + "." + trGrp + "." + trName
                    : callAlias;
                this->registerTrigger(
                    m_triggers.getTrigger(trNsp, trGrp, trName), callbackName);
                m_triggers.getTrigger(trNsp, trGrp, trName)
                    .lock()
                    ->registerEnvironment(m_id, m_environment, callbackName, &m_active);
            }
            else
            {
                const std::string callbackName = (callAlias.empty())
                    ? trNsp + "." + trGrp + "." + trName
                    : callAlias;
                m_triggers.getTrigger(trNsp, trGrp, trName)
                    .lock()
                    ->unregisterEnvironment(m_environment);
                m_triggers.getTrigger(trNsp, trGrp, trName)
                    .lock()
                    ->registerEnvironment(m_id, m_environment, callbackName, &m_active);
            }
        }
    }

    void GameObject::removeTrigger(const std::string& trNsp, const std::string& trGrp,
        const std::string& trName) const
    {
        m_triggers.getTrigger(trNsp, trGrp, trName)
            .lock()
            ->unregisterEnvironment(m_environment);
    }

    void GameObject::exec(const std::string& query)
    {
        m_lua.safe_script(query, m_environment);
    }

    void GameObject::deleteObject()
    {
        Debug::Log->debug(
            "GameObject::deleteObject called for '{0}' ({1})", m_id, m_type);
        if (m_hasScriptEngine)
            t_local->trigger("Delete");
        this->deletable = true;
        m_active = false;
        if (m_hasScriptEngine)
        {
            for (auto& triggerRef : m_registeredTriggers)
            {
                if (auto trigger = triggerRef.first.lock())
                {
                    trigger->unregisterEnvironment(m_environment);
                }
            }
            for (const auto& trigger : t_local->getTriggers())
            {
                m_environment["__TRIGGERS"][trigger->getTriggerLuaTableName()]
                    = sol::lua_nil;
            }
        }
    }

    void GameObject::setPermanent(bool permanent)
    {
        m_permanent = permanent;
    }

    bool GameObject::isPermanent() const
    {
        return m_permanent;
    }

    sol::environment GameObject::getEnvironment() const
    {
        return m_environment;
    }

    void GameObject::setState(bool state)
    {
        m_active = state;
    }
} // namespace obe::Script