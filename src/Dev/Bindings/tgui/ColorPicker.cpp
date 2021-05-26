#include <Bindings/tgui/tgui.hpp>

#include <TGUI/Widgets/ColorPicker.hpp>

#include <Bindings/Config.hpp>

namespace tgui::Bindings
{
    void LoadClassColorPicker(sol::state_view state)
    {
        sol::table tguiNamespace = state["tgui"].get<sol::table>();
        sol::usertype<tgui::ColorPicker> bindColorPicker
            = tguiNamespace.new_usertype<tgui::ColorPicker>("ColorPicker",
                sol::call_constructor,
                sol::constructors<tgui::ColorPicker(), tgui::ColorPicker(const char*),
                    tgui::ColorPicker(const char*, bool),
                    tgui::ColorPicker(const tgui::ColorPicker&)>(),
                sol::base_classes,
                sol::bases<tgui::ChildWindow, tgui::Container, tgui::Widget>());
        bindColorPicker["operator="]
            = sol::overload(static_cast<tgui::ColorPicker& (
                                tgui::ColorPicker::*)(const tgui::ColorPicker&)>(
                                &tgui::ColorPicker::operator=),
                [](tgui::ColorPicker* self, tgui::ColorPicker right) {
                    self->operator=(std::move(right));
                });
        bindColorPicker["getSharedRenderer"] = sol::overload(
            static_cast<tgui::ColorPickerRenderer* (tgui::ColorPicker::*)()>(
                &tgui::ColorPicker::getSharedRenderer),
            static_cast<const tgui::ColorPickerRenderer* (tgui::ColorPicker::*)() const>(
                &tgui::ColorPicker::getSharedRenderer));
        bindColorPicker["getRenderer"] = sol::overload(
            static_cast<tgui::ColorPickerRenderer* (tgui::ColorPicker::*)()>(
                &tgui::ColorPicker::getRenderer),
            static_cast<const tgui::ColorPickerRenderer* (tgui::ColorPicker::*)() const>(
                &tgui::ColorPicker::getRenderer));
        bindColorPicker["setColor"] = &tgui::ColorPicker::setColor;
        bindColorPicker["getColor"] = &tgui::ColorPicker::getColor;
        bindColorPicker["leftMousePressed"] = &tgui::ColorPicker::leftMousePressed;
        bindColorPicker["leftMouseButtonNoLongerDown"]
            = &tgui::ColorPicker::leftMouseButtonNoLongerDown;
        bindColorPicker["mouseMoved"] = &tgui::ColorPicker::mouseMoved;
        bindColorPicker["draw"] =

            [](tgui::ColorPicker* self, tgui::BackendRenderTargetBase* target,
                tgui::RenderStates states) { self->draw(*target, states); }

        ;
        bindColorPicker["create"] = sol::overload(
            [](tgui::ColorPicker* self) -> tgui::ColorPicker::Ptr {
                return self->create();
            },
            [](tgui::ColorPicker* self, tgui::String title) -> tgui::ColorPicker::Ptr {
                return self->create(title);
            },
            [](tgui::ColorPicker* self, tgui::String title, tgui::Color color)
                -> tgui::ColorPicker::Ptr { return self->create(title, color); });
        bindColorPicker["copy"] = &tgui::ColorPicker::copy;
        bindColorPicker["onColorChange"] = &tgui::ColorPicker::onColorChange;
        bindColorPicker["onOkPress"] = &tgui::ColorPicker::onOkPress;
    }
};