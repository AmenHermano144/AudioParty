#include "../../../includes.hh"

namespace framework
{
	c_slider_float::c_slider_float(std::string label, float* val, float min, float max, bool hide_label, std::wstring prefix) : m_val(val), m_min(min), m_max(max), m_prefix(prefix)
	{
		m_label = std::move(label);
		m_hide_label = hide_label;

		m_size = { 0, (m_hide_label ? 0 : g_font->f_childs.measure(m_label).y) + 15 };
		m_type = element_type::slider;
		m_focus_priority = focus_priority::persistent;

		// we only use this in terms of inlining elements
		m_parent_width = m_child_size;
	}

	void c_slider_float::input()
	{
		auto position = m_hide_label ? math::c_vector_2d(0, 0) : math::c_vector_2d(0, g_font->f_childs.measure(m_label).y + 5);
		math::c_rect bounding = math::c_rect(m_pos + math::c_vector_2d(0, position.y), math::c_vector_2d(m_child_size, 10.f));

		if (!g_ctx->can_interact(this, m_focus_priority))
			return;
			
		// check if hovered is nullptr and then if we are in region
		if (g_input->mouse_in_region(bounding.pos(), bounding.size()))
			g_ctx->m_hovered = this;
		else if (g_ctx->m_hovered == this)
		{
			g_ctx->m_hovered = nullptr;
		}

		if (g_input->clicked(input::mouse_buttons::left) && g_ctx->m_hovered == this)
			g_ctx->push_focus(this, m_focus_priority);

		if (g_input->click_down(input::mouse_buttons::left)) {
			if (g_ctx->is_focused(this))
			{
				float offset = std::clamp<float>(math::c_vector_2d(g_input->get_mouse_position() - this->m_pos).x, 0, m_child_size);
				float target_value = utils::builder::modulate_float(offset, 0, m_child_size, this->m_min, this->m_max);

				/* update value */
				*this->m_val = *this->m_val + (target_value - *this->m_val) * 0.2f;
			}
		}

		// release focus on mouse up
		if (g_input->click_released(input::mouse_buttons::left) && g_ctx->is_focused(this))
			g_ctx->pop_focus(this);

	}

	void c_slider_float::draw()
	{
		animations::m_slider_opacity = utils::builder::create_animation_ctx(m_parent + m_label, m_visible && g_ctx->m_open, 0.5);
		animations::m_slider_value = utils::builder::create_animation_ctx(m_parent + m_label + "#m_slider_value", m_visible && (*this->m_val > this->m_min + 0.1) && g_ctx->m_open, 0.5);
		animations::m_slider_hover = utils::builder::create_animation_ctx(m_parent + m_label + "#m_slider_hover", m_visible && g_ctx->m_hovered == this, 0.5);

		// animation handling
		float target_opacity = 0.2f;
		if (animations::m_slider_value.val() > 0.f)
			target_opacity = 0.2f + (0.6 * animations::m_slider_value.val());
		else if (animations::m_slider_hover.val() > 0.f)
			target_opacity = 0.2f + (0.2f * animations::m_slider_hover.val());

		static std::unordered_map<std::string, float> smooth_opacity_cache;
		std::string opacity_key = m_parent + m_label + "#smooth_opacity";
		float& smooth_opacity = smooth_opacity_cache[opacity_key];

		float lerp_speed = 0.3f;
		smooth_opacity += (target_opacity - smooth_opacity) * lerp_speed;
		float final_opacity = animations::m_slider_opacity.val() * smooth_opacity;

		auto position = m_hide_label ? math::c_vector_2d(0, 0) : math::c_vector_2d(0, g_font->f_childs.measure(m_label).y + 5);
		auto position2 = m_hide_label ? math::c_vector_2d(0, 0) : math::c_vector_2d(0, g_font->f_childs.measure(m_label).y + 7);

		g_render->use_layer(m_layer, [&]()
			{
				if (!m_hide_label)
					g_font->f_childs.text(m_pos.x, m_pos.y - 0.5, m_label, g_style->m_text.modulate(final_opacity));

				g_render->rect_filled((m_pos + position).x, (m_pos + position).y, m_child_size, 10.f, g_style->m_element_base.modulate(animations::m_window_opacity.val()), 2.f);
				g_render->rect((m_pos + position).x, (m_pos + position).y, m_child_size, 10.f, g_style->m_outline.modulate(animations::m_window_opacity.val()), 2.f);

				float width = utils::builder::modulate_float(*this->m_val, this->m_min, this->m_max, 0, m_child_size);
				if (*this->m_val > this->m_min + 0.1)
				{
					auto schizo = m_pos + math::c_vector_2d(1, position2.y);

					g_render->rect_filled(schizo.x, schizo.y, width, 8.f, g_style->m_accent.modulate(animations::m_window_opacity.val()), 2.f);
					g_render->fade_rect_filled(schizo.x, schizo.y, width, 8.f, hue::c_color(0, 0, 0, 0), hue::c_color(0, 0, 0, 50 * animations::m_window_opacity.val()), engine::fade_direction::vertically, 2.f);

				}

				std::string info = utils::builder::precision(*this->m_val, 1) + " " + utils::builder::wstring_to_string(this->m_prefix.c_str());

				if (!m_hide_label)
					g_font->f_childs.text((m_pos + math::c_vector_2d(m_child_size - g_font->f_childs.measure(info).x, 0.5)).x, (m_pos + math::c_vector_2d(m_child_size - g_font->f_childs.measure(info).x, 0.5)).y,
						info, g_style->m_text.modulate(animations::m_window_opacity.limit(0.3f).val()));
				animations::m_window_opacity.restore();
			});
	}

	c_slider_int::c_slider_int(std::string label, int* val, int min, int max, bool hide_label, std::wstring prefix) : m_val(val), m_min(min), m_max(max), m_prefix(prefix)
	{
		m_label = std::move(label);
		m_hide_label = hide_label;

		m_size = { 0, (m_hide_label ? 0 : g_font->f_childs.measure(m_label).y) + 15 };
		m_type = element_type::slider;

		// we only use this in terms of inlining elements
		m_parent_width = m_child_size;
	}

	void c_slider_int::input()
	{
		auto position = m_hide_label ? math::c_vector_2d(0, 0) : math::c_vector_2d(0, g_font->f_childs.measure(m_label).y + 5);
		math::c_rect bounding = math::c_rect(m_pos + math::c_vector_2d(0, position.y), math::c_vector_2d(m_child_size, 10.f));

		if (!g_ctx->can_interact(this, m_focus_priority))
			return;

		if (g_input->mouse_in_region(bounding.pos(), bounding.size()))
		{
			g_ctx->m_hovered = this;
		}
		else if (g_ctx->m_hovered == this)
		{
			g_ctx->m_hovered = nullptr;
		}

		if (g_input->clicked(input::mouse_buttons::left) && g_ctx->m_hovered == this)
			g_ctx->push_focus(this, m_focus_priority);

		if (g_input->click_down(input::mouse_buttons::left)) {
			if (g_ctx->is_focused(this))
			{
				float offset = std::clamp<float>(math::c_vector_2d(g_input->get_mouse_position() - this->m_pos).x, 0, m_child_size);

				/* we are forcing the conversion to float here using (float) */
				float target_value = utils::builder::modulate_float(offset, 0, m_child_size, (float)this->m_min, (float)this->m_max);

				/* note for future if i ever stop again that much,
					animating an int will have to get rounded
				*/
				static float current = (float)(*this->m_val); /* we are converting to float */
				current += (target_value - current) * 0.2f;

				/* we apply rounding and then we sawp it for precision */
				*this->m_val = (int)(current + 0.5f);

				if (std::abs(current - target_value) < 0.5f)
					*this->m_val = (int)(target_value + 0.5f);

				/* update value */
				*this->m_val = *this->m_val + (target_value - *this->m_val) * 0.2f;
			}		
		}


		// release focus on mouse up
		if (g_input->click_released(input::mouse_buttons::left) && g_ctx->is_focused(this))
			g_ctx->pop_focus(this);
	}

	void c_slider_int::draw()
	{
		animations::m_slider_opacity = utils::builder::create_animation_ctx(m_parent + m_label, m_visible && g_ctx->m_open, 0.5);
		animations::m_slider_value = utils::builder::create_animation_ctx(m_parent + m_label + "#m_slider_int_value", m_visible && (*this->m_val > this->m_min) && g_ctx->m_open, 0.5);
		animations::m_slider_hover = utils::builder::create_animation_ctx(m_parent + m_label + "#m_slider_int_hover", m_visible && g_ctx->m_hovered == this, 0.5);

		// animation handling
		float target_opacity = 0.2f;
		if (animations::m_slider_value.val() > 0.f)
			target_opacity = 0.2f + (0.6 * animations::m_slider_value.val());
		else if (animations::m_slider_hover.val() > 0.f)
			target_opacity = 0.2f + (0.2f * animations::m_slider_hover.val());

		static std::unordered_map<std::string, float> smooth_opacity_cache;
		std::string opacity_key = m_parent + m_label + "#smooth_opacity";
		float& smooth_opacity = smooth_opacity_cache[opacity_key];

		float lerp_speed = 0.3f;
		smooth_opacity += (target_opacity - smooth_opacity) * lerp_speed;
		float final_opacity = animations::m_slider_opacity.val() * smooth_opacity;

		auto position = m_hide_label ? math::c_vector_2d(0, 0) : math::c_vector_2d(0, g_font->f_childs.measure(m_label).y + 5);
		auto position2 = m_hide_label ? math::c_vector_2d(0, 0) : math::c_vector_2d(0, g_font->f_childs.measure(m_label).y + 7);

		g_render->use_layer(m_layer, [&]()
			{
				if (!m_hide_label)
					g_font->f_childs.text(m_pos.x, m_pos.y - 0.5, m_label, g_style->m_text.modulate(final_opacity));

				g_render->rect_filled((m_pos + position).x, (m_pos + position).y, m_child_size, 10.f, g_style->m_element_base.modulate(animations::m_window_opacity.val()), 2.f);
				g_render->rect((m_pos + position).x, (m_pos + position).y, m_child_size, 10.f, g_style->m_outline.modulate(animations::m_window_opacity.val()), 2.f);

				float width = utils::builder::modulate_float(*this->m_val, this->m_min, this->m_max, 0, m_child_size);
				if (*this->m_val > this->m_min + 1)
				{
					auto schizo = m_pos + math::c_vector_2d(1, position2.y);

					g_render->rect_filled(schizo.x, schizo.y, width, 8.f, g_style->m_accent.modulate(animations::m_window_opacity.val()), 2.f);
					g_render->fade_rect_filled(schizo.x, schizo.y, width, 8.f, hue::c_color(0, 0, 0, 0), hue::c_color(0, 0, 0, 50 * animations::m_window_opacity.val()), engine::fade_direction::vertically, 2.f);
				}

				std::string info = utils::builder::precision(*this->m_val, 1) + " " + utils::builder::wstring_to_string(this->m_prefix.c_str());

				if (!m_hide_label)
					g_font->f_childs.text((m_pos + math::c_vector_2d(m_child_size - g_font->f_childs.measure(info).x, 0.5)).x, (m_pos + math::c_vector_2d(m_child_size - g_font->f_childs.measure(info).x, 0.5)).y,
						info, g_style->m_text.modulate(animations::m_window_opacity.limit(0.3f).val()));
				animations::m_window_opacity.restore();
			});
	}
}