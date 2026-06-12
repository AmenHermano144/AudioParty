#include "../../../includes.hh"

namespace framework
{
	c_child::c_child(std::string name, child_width width, float y) : m_name(std::move(name)), m_width_type(width), m_size(math::c_vector_2d(0, y)), m_titlebar(true) {}

	void c_child::draw()
	{
		g_render->rect_shadow(this->m_pos.x, this->m_pos.y, this->m_size.x, this->m_size.y, hue::c_color(5, 5, 5).modulate(animations::m_window_opacity.val()), 30.f, 6.f);
		g_render->rect_filled(this->m_pos.x, this->m_pos.y, this->m_size.x, this->m_size.y, g_style->m_window_bars.modulate(animations::m_window_opacity.val()), 6);

		if (this->m_titlebar)
		{
			g_render->rect_filled(this->m_pos.x + 1, this->m_pos.y + 1, this->m_size.x - 2, 30, g_style->m_child_top.modulate(animations::m_window_opacity.val()), 6, engine::draw_flags_::draw_flags_round_corners_top);
			g_font->f_default.text(this->m_pos.x + 10, this->m_pos.y + 7, this->m_name, g_style->m_text.modulate(animations::m_window_opacity.limit(0.6).val()));
		}

		// element handling
		float padding = 0.f;
		c_base_element* m_parent_control = nullptr;

		math::c_vector_2d content_start = this->m_pos + this->calculate_element_padding();
		float content_area_height = this->m_size.y - this->calculate_element_padding().y;

		for (auto& control : m_controls)
		{
			// if control is inlined we use parent's control data to init this one
			if (control->m_inlined && m_parent_control != nullptr) {
				// we have to check if the element is a slider since it needs abit more presuring on it
				if (control->m_type == element_type::slider)
				{
					// if its slider we also have to lower out the m_child_size
					int base_scalling = (this->m_size.x - 20.f);

					// check if parent is slider
					if (m_parent_control->m_type == element_type::slider)
					{
						m_parent_control->m_child_size = m_parent_control->m_parent_width = (base_scalling * 0.5) - 7.5;
					}

					control->m_child_size = (base_scalling * 0.5) - 7.5;

					// hide label if the parent control is a checkbox
					if (m_parent_control->m_type == element_type::checkbox && !control->m_hide_label)
					{
						// force label hide
						control->hide_label();
					}

					// check if parent control is checkbox so we align it when we have no label
					if (m_parent_control->m_type == element_type::checkbox && control->m_hide_label)
					{
						control->m_pos = m_parent_control->m_pos + math::c_vector_2d(m_parent_control->m_parent_width + 15.f, 4);
					}
					else {
						control->m_pos = m_parent_control->m_pos + math::c_vector_2d(m_parent_control->m_parent_width + 15.f, 0);
					}
				}
				else if (control->m_type == element_type::button)
				{
					// check if parent is not a button, so we dont allow it
					if (m_parent_control->m_type != element_type::button)
					{
						// do not allow inlining
						control->m_inlined = false;
						continue;
					}

					// if its slider we also have to lower out the m_child_size
					int base_scalling = (this->m_size.x - 20.f);

					m_parent_control->m_child_size = m_parent_control->m_parent_width = (base_scalling * 0.5) - 7.5;
					control->m_child_size = (base_scalling * 0.5) - 7.5;

					control->m_pos = m_parent_control->m_pos + math::c_vector_2d(m_parent_control->m_parent_width + 10.f, 0);
				}
				else if (control->m_type == element_type::text_input)
				{
					// check if parent is not a button, so we dont allow it
					if (m_parent_control->m_type != element_type::text_input)
					{
						// do not allow inlining
						control->m_inlined = false;
						continue;
					}

					// if its slider we also have to lower out the m_child_size
					int base_scalling = (this->m_size.x - 20.f);

					m_parent_control->m_child_size = m_parent_control->m_parent_width = (base_scalling * 0.5) - 7.5;
					control->m_child_size = (base_scalling * 0.5) - 7.5;

					control->m_pos = m_parent_control->m_pos + math::c_vector_2d(m_parent_control->m_parent_width + 10.f, 0);
				}
				else if (control->m_type == element_type::colorpicker)
				{
					// if the colorpicker is inlined disabled label
					control->hide_label();

					// if we are inlining to a checkbox we are setting parent + pos - icon
					if (m_parent_control->m_type == element_type::checkbox)
					{
						control->m_pos = m_parent_control->m_pos + math::c_vector_2d((control->m_child_size - g_font->f_icons.measure(ICON_FA_PALETTE).x) - 2, 0);
					} // down from here is multiinlining
					else if (m_parent_control->m_type == element_type::colorpicker)
					{
						control->m_pos = m_parent_control->m_pos - math::c_vector_2d(m_parent_control->m_parent_width + 5.f, 0);
					}
				}
				else if (control->m_type == element_type::keybind)
				{
					// if the colorpicker is inlined disabled label
					control->hide_label();

					// if we are inlining to a checkbox we are setting parent + pos - icon
					if (m_parent_control->m_type == element_type::checkbox)
					{
						control->m_pos = m_parent_control->m_pos + math::c_vector_2d((control->m_child_size - g_font->f_icons.measure(ICON_FA_KEYBOARD).x) - 2, 0);
					} // down from here is multiinlining
					else if (m_parent_control->m_type == element_type::colorpicker)
					{
						control->m_pos = m_parent_control->m_pos - math::c_vector_2d(m_parent_control->m_parent_width + 8.f, 0);
					}
				}
				else if (control->m_type == element_type::popup)
				{
					// if the colorpicker is inlined disabled label
					control->hide_label();

					// if we are inlining to a checkbox we are setting parent + pos - icon
					if (m_parent_control->m_type == element_type::checkbox)
					{
						control->m_pos = m_parent_control->m_pos + math::c_vector_2d((control->m_child_size - g_font->f_icons.measure(ICON_FA_SQUARE_PLUS).x) - 2, 0);
					} // down from here is multiinlining
					else if (m_parent_control->m_type == element_type::colorpicker)
					{
						control->m_pos = m_parent_control->m_pos - math::c_vector_2d(m_parent_control->m_parent_width + 8.f, 0);
					}
				}
				else {
					control->m_pos = m_parent_control->m_pos + math::c_vector_2d(m_parent_control->m_parent_width + 15.f, 0);
				}
			}
			else {
				control->m_pos = this->m_pos + math::c_vector_2d(0, padding - m_scroll_offset) + this->calculate_element_padding();
			}

			if (!this->visible())
			{
				continue;
			}

			// callback visibility
			if (control->m_callback_visibility && control->m_visible_by_callback && !(*control->m_visible_by_callback)) {
				continue;
			}

			float control_top = control->m_pos.y;
			float control_bottom = control->m_pos.y + control->m_size.y;
			float visible_top = content_start.y;
			float visible_bottom = this->m_pos.y + this->m_size.y - 12.f;

			bool is_visible = !(control_bottom < visible_top || control_top > visible_bottom);

			// c_base_control::base
			// set the child parent, we are going to use this in the checkbox data ( if there are problems, make sure to include, tab subtab )
			control->set_parent(this->m_name + "#" + this->m_child_attach_data.m_subtab_name);

			// set base control visibility based on where we are
			control->set_visibility(this->visible() && is_visible);

			// c_base_control->element
			// no point in inputting if we have no menu opened
			if (g_ctx->m_open && is_visible)
			{
				// if a modal is open, only allow the popup that owns it to run input
				// everything else behind it gets blocked
				if (g_ctx->m_modal_owner != nullptr && control.get() != g_ctx->m_modal_owner)
				{
					// skip input for background elements
				}
				else if (g_ctx->can_interact(control.get(), control->m_focus_priority))
				{
					control->input();
				}
			}

			// engine::c_layout_engine(this->m_pos + this->calculate_element_padding(), this->calculate_safe_area())
			if (control->m_type != element_type::popup)
				g_render->push_clip((this->m_pos + this->calculate_element_padding()).x, (this->m_pos + this->calculate_element_padding()).y, this->calculate_safe_area().x, this->calculate_safe_area().y);

			if (is_visible)
			{
				control->draw();
			}

			if (control->m_type != element_type::popup)
				g_render->restore_clip();

			// push y only if the control is not inlined
			if (!control->m_inlined)
				padding += control->m_size.y + 8.f;

			// we only set this if we do inlining
			control->m_parent_control = m_parent_control;
			m_parent_control = control.get();

			// set child data to control base
			control->m_child_size = this->m_size.x - 25.f;

			// g_ctx->m_focus_took = control.get();

			// check which control is focused
			if (g_ctx->m_focus_took != nullptr)
				slog::log::debug("[framework::c_child] control focused: {}", g_ctx->m_focus_took->m_label);

			animations::m_window_opacity.restore();
		}

		m_content_height = padding;
		m_max_scroll = std::max(0.f, m_content_height - content_area_height);

		// scrollbar
		if (m_max_scroll > 0.f && this->visible())
		{
			float track_width = 4.f;
			float track_padding = 4.f;
			float titlebar_offset = this->m_titlebar ? 32.f : 4.f;

			float track_x = this->m_pos.x + this->m_size.x - track_width - track_padding;
			float track_y = this->m_pos.y + titlebar_offset;
			float track_h = this->m_size.y - titlebar_offset - track_padding;

			// thumb size proportional to visible area
			float visible_ratio = content_area_height / m_content_height;
			float thumb_h = std::max(20.f, track_h * visible_ratio);
			float scroll_ratio = m_scroll_offset / m_max_scroll;
			float thumb_y = track_y + scroll_ratio * (track_h - thumb_h);

			// fade scrollbar based on hover/drag
			bool is_hovered = g_input->mouse_in_region(
				math::c_vector_2d(track_x - 4.f, track_y),
				math::c_vector_2d(track_width + 8.f, track_h));

			float target_opacity = (is_hovered || m_scrollbar_dragging) ? 1.f : 0.4f;
			m_scrollbar_opacity += (target_opacity - m_scrollbar_opacity) * 0.15f;

			float alpha = animations::m_window_opacity.val() * m_scrollbar_opacity;

			// track
			g_render->rect_filled(
				static_cast<int>(track_x), static_cast<int>(track_y),
				static_cast<int>(track_width), static_cast<int>(track_h),
				hue::c_color(255, 255, 255, static_cast<int>(15 * alpha)), 2.f);

			// thumb
			auto thumb_color = (is_hovered || m_scrollbar_dragging)
				? g_style->m_accent.modulate(alpha)
				: hue::c_color(255, 255, 255, static_cast<int>(80 * alpha));

			g_render->rect_filled(
				static_cast<int>(track_x), static_cast<int>(thumb_y),
				static_cast<int>(track_width), static_cast<int>(thumb_h),
				thumb_color, 2.f);
		}

		// reset it
		animations::m_window_opacity.restore();
	}

	void c_child::input()
	{
		bool is_hovered = g_input->mouse_in_region(this->m_pos, this->m_size);
		if (is_hovered && this->visible() && m_max_scroll > 0.f)
		{
			float scroll_speed = 40.f;
			float scroll_delta = g_input->get_wheel_value();

			m_scroll_target -= scroll_delta * scroll_speed;
			m_scroll_target = std::clamp(m_scroll_target, 0.f, m_max_scroll);
		}

		// scrollbar drag
		if (this->visible() && m_max_scroll > 0.f)
		{
			float track_width = 4.f;
			float track_padding = 4.f;
			float titlebar_offset = this->m_titlebar ? 32.f : 4.f;

			float track_x = this->m_pos.x + this->m_size.x - track_width - track_padding;
			float track_y = this->m_pos.y + titlebar_offset;
			float track_h = this->m_size.y - titlebar_offset - track_padding;

			float content_area_height = this->m_size.y - this->calculate_element_padding().y;
			float visible_ratio = content_area_height / m_content_height;
			float thumb_h = std::max(20.f, track_h * visible_ratio);

			bool scrollbar_hovered = g_input->mouse_in_region(
				math::c_vector_2d(track_x - 4.f, track_y),
				math::c_vector_2d(track_width + 8.f, track_h));

			bool mouse_down = g_input->click_down(input::left);

			if (scrollbar_hovered && mouse_down && !m_scrollbar_dragging)
				m_scrollbar_dragging = true;

			if (!mouse_down)
				m_scrollbar_dragging = false;

			if (m_scrollbar_dragging)
			{
				float mouse_y = g_input->get_mouse_position().y;
				float relative = (mouse_y - track_y - thumb_h * 0.5f) / (track_h - thumb_h);
				relative = std::clamp(relative, 0.f, 1.f);
				m_scroll_target = relative * m_max_scroll;
			}
		}

		float lerp_speed = 0.15f;
		m_scroll_offset += (m_scroll_target - m_scroll_offset) * lerp_speed;

		if (std::abs(m_scroll_target - m_scroll_offset) < 0.5f)
		{
			m_scroll_offset = m_scroll_target;
		}
	}

	void c_child::attach_child(std::string tab_name, std::string subtab_name)
	{
		this->m_child_attach_data.m_tab_name = tab_name;
		this->m_child_attach_data.m_subtab_name = subtab_name;
	}

	void c_child::set_visible(bool data)
	{
		this->m_visible = data;
	}

	bool c_child::visible()
	{
		if (g_ctx->m_cur_tab != this->m_child_attach_data.m_tab_name)
		{
			return false;
		}

		bool same_tab = g_ctx->m_cur_tab == this->m_child_attach_data.m_tab_name;
		if (same_tab && g_ctx->m_tabs[g_ctx->m_active_tab].m_subtab.empty())
		{
			return true;
		}

		if (same_tab && g_ctx->m_tabs[g_ctx->m_active_tab].m_cur_subtab != this->m_child_attach_data.m_subtab_name)
		{
			return false;
		}

		if (same_tab && g_ctx->m_tabs[g_ctx->m_active_tab].m_cur_subtab == this->m_child_attach_data.m_subtab_name)
		{
			return true;
		}

		// if none of the conditions matched return false
		return false;
	}

	child_width c_child::get_type()
	{
		return this->m_width_type;
	}

	math::c_vector_2d c_child::calculate_element_padding()
	{
		if (this->m_titlebar)
		{
			return math::c_vector_2d(12.f, 42.f);
		}
		else {
			return math::c_vector_2d(10.f, 12.f);
		}
	}

	math::c_vector_2d c_child::calculate_safe_area()
	{
		if (this->m_titlebar)
		{
			return this->m_size - math::c_vector_2d(18.f, 50.f);
		}
		else {
			return this->m_size - math::c_vector_2d(20.f, 24.f);
		}
	}

	std::shared_ptr<framework::c_checkbox> c_child::add_checkbox(std::string label, bool* val)
	{
		auto control = std::make_shared<framework::c_checkbox>(label, val);
		{
			// cache this new added element
			this->m_controls.push_back(control);

			slog::log::success("[framework::c_child] created a new control: [type: c_checkbox, label: {}]", label);
		}

		// allow chaining (->colorpicker(), ->popup())
		return control;
	}

	std::shared_ptr<c_slider_float> c_child::add_slider_float(std::string label, float* val, float min, float max, bool hide_label, std::wstring prefix)
	{
		auto control = std::make_shared<framework::c_slider_float>(label, val, min, max, hide_label, prefix);
		{
			// cache this new added element
			this->m_controls.push_back(control);

			slog::log::success("[framework::c_child] created a new control: [type: c_slider_float, label: {}]", label);
		}

		// allow chaining (->colorpicker(), ->popup())
		return control;
	}

	std::shared_ptr<c_slider_int> c_child::add_slider_int(std::string label, int* val, int min, int max, bool hide_label, std::wstring prefix)
	{
		auto control = std::make_shared<framework::c_slider_int>(label, val, min, max, hide_label, prefix);
		{
			// cache this new added element
			this->m_controls.push_back(control);

			slog::log::success("[framework::c_child] created a new control: [type: c_slider_int, label: {}]", label);
		}

		// allow chaining (->colorpicker(), ->popup())
		return control;
	}

	std::shared_ptr<c_dropdown> c_child::add_dropdown(std::string label, int* val, std::vector<std::string> items, bool hide_label)
	{
		auto control = std::make_shared<c_dropdown>(label, val, items, hide_label);
		{
			this->m_controls.push_back(control);

			slog::log::success("[framework::c_child] created a new control: [type: c_dropdown, label: {}]", label);
		}

		// allow chaining
		return control;
	}

	std::shared_ptr<c_multidropdown> c_child::add_multibox(std::string label, bool hide_label, std::function<void(c_multidropdown* ptr)> callback)
	{
		auto control = std::make_shared<c_multidropdown>(label, hide_label);
		{
			callback(control.get());

			this->m_controls.push_back(control);

			slog::log::success("[framework::c_child] created a new control: [type: c_multidropdown, label: {}]", label);
		}

		return control;
	}

	std::shared_ptr<c_button> c_child::add_button(std::string label, std::function<void()> callback)
	{
		auto control = std::make_shared<c_button>(label, callback);
		{
			this->m_controls.push_back(control);
			slog::log::success("[framework::c_child] created a new control: [type: c_button, label: {}]", label);
		}

		return control;
	}

	std::shared_ptr<c_colorpicker> c_child::add_colorpicker(std::string label, hue::c_color* val, bool hide_label)
	{
		auto control = std::make_shared<c_colorpicker>(label, val, hide_label);
		{
			this->m_controls.push_back(control);
			slog::log::success("[framework::c_child] created a new control: [type: c_colorpicker, label: {}]", label);
		}

		return control;
	}

	std::shared_ptr<c_keybind> c_child::add_keybind(std::string label, key_var_t* val, bool hide_label)
	{
		auto control = std::make_shared<c_keybind>(label, val, hide_label);
		{
			this->m_controls.push_back(control);
			slog::log::success("[framework::c_child] created a new control: [type: c_keybind, label: {}]", label);
		}

		return control;
	}

	std::shared_ptr<c_text_input> c_child::add_input_box(std::string label, std::string* val, bool hide_label)
	{
		auto control = std::make_shared<c_text_input>(label, val, hide_label);
		{
			this->m_controls.push_back(control);
			slog::log::success("[framework::c_child] created a new control: [type: c_text_input, label: {}]", label);
		}
		return control;
	}

	std::shared_ptr<c_popup> c_child::add_popup(std::string label, bool hide_label, std::function<void(c_popup* ptr)> callback)
	{
		auto control = std::make_shared<c_popup>(label, hide_label);
		{
			callback(control.get());

			this->m_controls.push_back(control);
			slog::log::success("[framework::c_child] created a new control: [type: c_popup, label: {}]", label);
		}

		return control;
	}

	std::shared_ptr<c_listbox> c_child::add_listbox(std::string label, int* val, std::vector<std::string> items, float height, bool hide_label)
	{
		auto control = std::make_shared<c_listbox>(label, val, items, height, hide_label);
		{
			this->m_controls.push_back(control);
			slog::log::success("[framework::c_child] created a new control: [type: c_listbox, label: {}]", label);
		}

		return control;
	}
}