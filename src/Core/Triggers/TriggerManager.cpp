#include <vili/ErrorHandler.hpp>

#include <Triggers/TriggerManager.hpp>

namespace obe::Triggers
{
    TriggerManager::TriggerManager()
        : Registrable("TriggerManager")
    {
        Debug::Log->debug("<TriggerManager> Initialising TriggerManager");
        this->createNamespace("Global");
        m_databaseChrono.start();
    }

    std::weak_ptr<Trigger> TriggerManager::getTrigger(
        const std::string& space, const std::string& group, const std::string& trigger)
    {
        if (m_allTriggers.find(space) != m_allTriggers.end())
        {
            if (m_allTriggers[space].find(group) != m_allTriggers[space].end())
                return m_allTriggers[space][group]->getTrigger(trigger);
            throw aube::ErrorHandler::Raise(
                "ObEngine.Triggers.TriggerManager.UnknownCustomTriggerGroup",
                { { "function", "getTrigger" }, { "group", group },
                    { "namespace", space } });
        }
        throw aube::ErrorHandler::Raise(
            "ObEngine.Trigger.TriggerManager.UnknownNamespace",
            { { "function", "getTrigger" }, { "namespace", space } });
    }

    void TriggerManager::createNamespace(const std::string& space)
    {
        Debug::Log->debug(
            "<TriggerManager> Creating Namespace {0} in TriggerManager", space);
        if (m_allTriggers.size() == 0)
            m_allTriggers[space] = std::map<std::string, std::unique_ptr<TriggerGroup>>();
        else
        {
            if (m_allTriggers.find(space) == m_allTriggers.end())
                m_allTriggers[space]
                    = std::map<std::string, std::unique_ptr<TriggerGroup>>();
            else
                throw aube::ErrorHandler::Raise(
                    "ObEngine.Triggers.TriggerManager.NamespaceAlreadyExists",
                    { { "namespace", space } });
        }
    }

    TriggerGroupPtr TriggerManager::createTriggerGroup(
        const std::string& space, const std::string& group)
    {
        Debug::Log->debug(
            "<TriggerManager> Creating TriggerGroup {0} in Namespace {1}", group, space);
        if (m_allTriggers.find(space) != m_allTriggers.end())
        {
            if (m_allTriggers[space].find(group) == m_allTriggers[space].end())
            {
                m_allTriggers[space][group]
                    = std::make_unique<TriggerGroup>(space, group);
                return TriggerGroupPtr(
                    m_allTriggers[space][group].get(), [this](TriggerGroup* ptr) {
                        this->removeTriggerGroup(ptr);
                    });
                ;
            }
            throw aube::ErrorHandler::Raise(
                "ObEngine.Triggers.TriggerManager.TriggerGroupAlreadyExists",
                { { "group", group }, { "namespace", space } });
        }
        throw aube::ErrorHandler::Raise(
            "ObEngine.Triggers.TriggerManager.UnknownNamespace",
            { { "function", "createTriggerGroup" }, { "namespace", space } });
    }

    TriggerGroupPtr TriggerManager::joinTriggerGroup(
        const std::string& space, const std::string& group)
    {
        Debug::Log->debug(
            "<TriggerManager> Joining TriggerGroup {0} in Namespace {1}", group, space);
        if (m_allTriggers.find(space) != m_allTriggers.end())
        {
            if (m_allTriggers[space].find(group) != m_allTriggers[space].end()
                && m_allTriggers[space][group]->isJoinable())
                return TriggerGroupPtr(
                    m_allTriggers[space][group].get(), [this](TriggerGroup* ptr) {
                        this->removeTriggerGroup(ptr);
                    });
            if (m_allTriggers[space].find(group) != m_allTriggers[space].end())
            {
                throw aube::ErrorHandler::Raise(
                    "ObEngine.Triggers.TriggerManager.TriggerGroupNotJoinable",
                    { { "group", group }, { "namespace", space } });
            }
            throw aube::ErrorHandler::Raise(
                "ObEngine.Triggers.TriggerManager.UnknownCustomTriggerGroup",
                { { "function", "joinTriggerGroup" }, { "group", group },
                    { "namespace", space } });
        }
        throw aube::ErrorHandler::Raise(
            "ObEngine.Triggers.TriggerManager.UnknownNamespace",
            { { "function", "joinTriggerGroup" }, { "namespace", space } });
    }

    void TriggerManager::removeNamespace(const std::string& space)
    {
        Debug::Log->debug("<TriggerManager> Removing Trigger Namespace {0}", space);
        if (m_allTriggers.find(space) != m_allTriggers.end())
        {
            Debug::Log->trace(
                "<TriggerManager> Found Trigger Namespace {0}, removing it...", space);
            m_allTriggers.erase(m_allTriggers.find(space));
        }
        else
            throw aube::ErrorHandler::Raise(
                "ObEngine.Triggers.TriggerManager.UnknownNamespace",
                { { "function", "removeNamespace" }, { "namespace", space } });
    }

    std::vector<std::string> TriggerManager::getAllTriggersNameFromTriggerGroup(
        const std::string& space, const std::string& group)
    {
        if (m_allTriggers.find(space) != m_allTriggers.end())
        {
            if (m_allTriggers[space].find(group) != m_allTriggers[space].end())
                return m_allTriggers[space][group]->getAllTriggersName();
            throw aube::ErrorHandler::Raise(
                "ObEngine.Triggers.TriggerManager.UnknownCustomTriggerGroup",
                { { "function", "getAllTriggersNameFromTriggerGroup" },
                    { "group", group }, { "namespace", space } });
        }
        throw aube::ErrorHandler::Raise(
            "ObEngine.Triggers.TriggerManager.UnknownNamespace",
            { { "function", "getAllTriggersNameFromTriggerGroup" },
                { "namespace", space } });
    }

    std::vector<std::string> TriggerManager::getAllTriggersGroupNames(
        const std::string& space)
    {
        if (m_allTriggers.find(space) != m_allTriggers.end())
        {
            std::vector<std::string> allNames;
            for (auto& space : m_allTriggers[space])
            {
                allNames.push_back(space.first);
            }
            return allNames;
        }
        else
            throw aube::ErrorHandler::Raise(
                "ObEngine.Triggers.TriggerManager.UnknownNamespace",
                { { "function", "getAllTriggersGroupNames" }, { "namespace", space } });
    }

    void TriggerManager::removeTriggerGroup(TriggerGroup* trgGroup)
    {
        Debug::Log->debug("<TriggerManager> Removing TriggerGroup {0} from Namespace {1}",
            trgGroup->getName(), trgGroup->getNamespace());
        m_allTriggers[trgGroup->getNamespace()].erase(trgGroup->getName());
    }

    bool TriggerManager::doesTriggerGroupExists(
        const std::string& space, const std::string& group)
    {
        if (m_allTriggers.find(space) != m_allTriggers.end())
        {
            if (m_allTriggers[space].find(group) == m_allTriggers[space].end())
                return false;
            return true;
        }
        throw aube::ErrorHandler::Raise(
            "ObEngine.Triggers.TriggerManager.UnknownNamespace",
            { { "function", "doesTriggerGroupExists" }, { "namespace", space } });
    }

    void TriggerManager::update()
    {
        Debug::Log->trace("<TriggerManager> Updating TriggerManager");
        for (auto it = m_allTriggers.begin(); it != m_allTriggers.end(); ++it)
        {
            for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
            {
                if (it2->second->m_delayedTriggers.size() != 0)
                {
                    for (int i = 0; i < it2->second->m_delayedTriggers.size(); i++)
                    {
                        it2->second->m_delayedTriggers[i]->m_delaytarget
                            = m_databaseChrono.getTime()
                            + it2->second->m_delayedTriggers[i]->m_delay;
                        m_delayedTriggers.push_back(
                            move(it2->second->m_delayedTriggers[i]));
                    }
                    it2->second->m_delayedTriggers.clear();
                }
            }
        }
        std::vector<int> triggeredDelayedTriggers;
        for (int i = 0; i < m_delayedTriggers.size(); i++)
        {
            if (m_delayedTriggers[i]->m_delaytarget <= m_databaseChrono.getTime())
            {
                m_delayedTriggers[i]->m_trigger->m_enabled = true;
                triggeredDelayedTriggers.push_back(i);
            }
        }
        for (int i = 0; i < triggeredDelayedTriggers.size(); i++)
        {
            m_delayedTriggers.erase(
                m_delayedTriggers.begin() + triggeredDelayedTriggers[i]);
        }
    }

    void TriggerManager::clear()
    {
        Debug::Log->debug("<TriggerManager> Clearing TriggerManager");
        m_delayedTriggers.clear();
        m_databaseChrono.stop();
        m_allTriggers.clear();
        m_databaseChrono.start();
        // Need to delete Map-only stuff !!
    }
} // namespace obe::Triggers