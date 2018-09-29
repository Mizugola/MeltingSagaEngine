#include <algorithm>

#include <Debug/Logger.hpp>
#include <Input/InputButtonMonitor.hpp>
#include <Input/KeyList.hpp>
#include <Triggers/TriggerDatabase.hpp>

namespace obe::Input
{
    Triggers::TriggerGroupPtr InputButtonMonitor::KeyTriggers;
    void InputButtonMonitor::InitKeyTriggerGroup()
    {
        InputButtonMonitor::KeyTriggers = Triggers::TriggerGroupPtr(
            Triggers::TriggerDatabase::GetInstance()->createTriggerGroup("Global", "Keys"), 
            Triggers::TriggerGroupPtrRemover
        );
    }

    InputButtonMonitor::InputButtonMonitor(InputButton* button)
    {
        m_button = button;
        InputButtonMonitor::KeyTriggers->addTrigger(m_button->getName());
    }

    InputButtonMonitor::~InputButtonMonitor()
    {
        InputButtonMonitor::KeyTriggers->removeTrigger(m_button->getName());
    }

    InputButton* InputButtonMonitor::getButton() const
    {
        return m_button;
    }

    InputButtonState InputButtonMonitor::getState() const
    {
        return m_buttonState;
    }

    void InputButtonMonitor::update()
    {
        const bool keyPressed = m_button->isPressed();
        InputButtonState oldState = m_buttonState;
        if (keyPressed && (m_buttonState == InputButtonState::Idle || m_buttonState == InputButtonState::Released))
        {
            m_buttonState = InputButtonState::Pressed;
            Monitors::RequireRefresh = true;
        }
        else if (keyPressed && m_buttonState == InputButtonState::Pressed)
        {
            m_buttonState = InputButtonState::Hold;
        }
        else if (!keyPressed && (m_buttonState == InputButtonState::Pressed || m_buttonState == InputButtonState::Hold))
        {
            m_buttonState = InputButtonState::Released;
            Monitors::RequireRefresh = true;
        }
        else if (!keyPressed && m_buttonState == InputButtonState::Released)
        {
            m_buttonState = InputButtonState::Idle;
        }
        if (oldState != m_buttonState)
        {
            InputButtonMonitor::KeyTriggers->pushParameter(m_button->getName(), "previousState", oldState);
            InputButtonMonitor::KeyTriggers->pushParameter(m_button->getName(), "state", m_buttonState);
            InputButtonMonitor::KeyTriggers->trigger(m_button->getName());
        }
    }

    void InputButtonMonitor::remove()
    {
        m_removed = true;
    }
    bool InputButtonMonitor::isRemoved() const
    {
        return m_removed;
    }

    void InputButtonMonitorPtrRemover(InputButtonMonitor* ptr)
    {
        ptr->remove();
    }

    namespace Monitors
    {
        std::vector<std::unique_ptr<InputButtonMonitor>> Monitors;
        bool RequireRefresh = true;
        void UpdateMonitors()
        {
            RequireRefresh = false;
            Monitors.erase(std::remove_if(Monitors.begin(), Monitors.end(), UpdateMonitorsAndRemoveIfNotRemoved), Monitors.end());
        }

        bool UpdateMonitorsAndRemoveIfNotRemoved(const std::unique_ptr<InputButtonMonitor>& element)
        {
            Debug::Log->warn("Updating {} : {}", element->getButton()->getName(), element->isRemoved());
            if (!element->isRemoved())
            {
                element->update();
                return false;
            }
            else
            {
                return true;
            }
        }

        InputButtonMonitorPtr Monitor(const std::string& buttonId)
        {
            return Monitor(GetKey(buttonId));
        }

        InputButtonMonitorPtr Monitor(InputButton* button)
        {
            for (auto& monitor : Monitors)
            {
                if (monitor->getButton() == button)
                {
                    return InputButtonMonitorPtr(monitor.get());
                }
            }
            Monitors.push_back(std::make_unique<InputButtonMonitor>(button));
            return InputButtonMonitorPtr(Monitors.back().get(), InputButtonMonitorPtrRemover);
        }
    }
}
