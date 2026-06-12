#include "../../../includes.hh"

namespace framework
{
	c_window::c_window(std::string title, math::c_vector_2d pos, math::c_vector_2d size) : m_title(title), m_pos(pos), m_size(size)
	{
		slog::log::success("[+] c_window:: {} created at pos: {}, {}, with size: {}, {}", m_title, m_pos.x, m_pos.y, m_size.x, m_size.y);
	}

	void c_window::paint()
	{
		// pin to top-left since the framework window IS the real window
		this->m_pos = math::c_vector_2d(0, 0);

		g_render->rect_shadow(this->m_pos.x, this->m_pos.y, this->m_size.x, this->m_size.y, g_style->m_window_shadow.modulate(animations::m_window_opacity.val()), 30.f, 6.f);
		g_render->rect_filled(this->m_pos.x, this->m_pos.y, this->m_size.x, this->m_size.y, g_style->m_window_background.modulate(animations::m_window_opacity.val()), 6.f);

		g_render->rect_filled(this->m_pos.x, this->m_pos.y, this->m_size.x, 35, g_style->m_window_bars.modulate(animations::m_window_opacity.val()), 6.f, engine::draw_flags_::draw_flags_round_corners_top);
		g_render->rect_filled(this->m_pos.x, this->m_pos.y + this->m_size.y - 35, this->m_size.x, 35, g_style->m_window_bars.modulate(animations::m_window_opacity.val()), 6.f, engine::draw_flags_::draw_flags_round_corners_bottom);

		// title
		g_font->f_default.text(this->m_pos.x + 10, this->m_pos.y + 8, this->m_title, g_style->m_text.modulate(animations::m_window_opacity.limit(0.6).val()));

		// close button (X) in top-right corner
		float close_x = this->m_pos.x + this->m_size.x - 30;
		float close_y = this->m_pos.y + 5;
		float close_w = 25;
		float close_h = 25;

		auto close_hovered = g_input->mouse_in_region(math::c_vector_2d(close_x, close_y), math::c_vector_2d(close_w, close_h));
		auto close_color = close_hovered ? hue::c_color(220, 60, 60) : g_style->m_text.modulate(0.5f);

		auto x_text = "x";
		auto x_size = g_font->f_bold.measure(x_text);
		g_font->f_bold.string(close_x + (close_w - x_size.x) * 0.5f, close_y + (close_h - x_size.y) * 0.5f, x_text, close_color.modulate(animations::m_window_opacity.val()));

		// bottom bar
		g_font->f_default.text(this->m_pos.x + 10, this->m_pos.y + this->m_size.y - (35 - 8), "AudioParty", g_style->m_text.modulate(animations::m_window_opacity.limit(0.6).val()))
			.inlined(" ~ ", g_style->m_text.modulate(animations::m_window_opacity.limit(0.2).val()))
			.inlined("v0.2", g_style->m_accent.modulate(animations::m_window_opacity.limit(0.8).val()));

		animations::m_window_opacity.restore();

		// paint tabs
		this->m_obj_tab->paint();

		// draw children
		math::c_vector_2d layout_system = {};
		float full_childrens = 0.f;

		for (auto& child : this->m_childrens)
		{
			if (child->get_type() == child_width::half)
			{
				float width_half = (this->m_size.x - (this->child_padding().x) - 24) * 0.5;
				child->m_size.x = width_half;
			}
			else if (child->get_type() == child_width::full)
			{
				float full_width = (this->m_size.x - (this->child_padding().x));
				child->m_size.x = full_width;
			}

			if (!child->visible())
			{
				child->m_relative_pos = {};
				child->m_pos = child->m_relative_pos + (this->m_pos + subtab_padding());
				continue;
			}

			float child_bottom = layout_system.y + child->m_size.y + (this->m_pos).y;
			if (child_bottom > this->m_pos.y + this->m_size.y && layout_system.y > 0)
			{
				layout_system.x += child->m_size.x + 15;
				layout_system.y = full_childrens;
			}

			child->m_relative_pos = layout_system;
			child->m_pos = child->m_relative_pos + (this->m_pos + subtab_padding());
			layout_system.y += child->m_size.y + 15;

			child->input();
			child->draw();

			if (child->get_type() == child_width::full)
			{
				full_childrens += child->m_size.y + 15;
			}
		}
	}

	void c_window::input()
	{
		// no toggle key needed — the window is always visible
		g_ctx->m_open = true;
		animations::m_window_opacity = utils::builder::create_animation_ctx(this->m_title, g_ctx->m_open, 0.5f);

		// close button hit test
		float close_x = this->m_pos.x + this->m_size.x - 30;
		float close_y = this->m_pos.y + 5;
		float close_w = 25;
		float close_h = 25;

		if (g_input->mouse_in_region(math::c_vector_2d(close_x, close_y), math::c_vector_2d(close_w, close_h))
			&& g_input->clicked(input::mouse_buttons::left))
		{
			if (g_ctx->m_hwnd)
				PostMessage(g_ctx->m_hwnd, WM_CLOSE, 0, 0);
			return;
		}

		// drag the real win32 window instead of the framework pos
		static POINT drag_start_cursor{};
		static RECT drag_start_rect{};

		math::c_rect bounding = math::c_rect(this->m_pos.x, this->m_pos.y, this->m_size.x, 35);

		if (!g_ctx->m_dragging
			&& g_input->mouse_in_region(bounding.pos(), bounding.size())
			&& g_input->click_down(input::mouse_buttons::left))
		{
			g_ctx->m_dragging = true;
			GetCursorPos(&drag_start_cursor);
			if (g_ctx->m_hwnd)
				GetWindowRect(g_ctx->m_hwnd, &drag_start_rect);
		}
		else if (g_ctx->m_dragging && g_input->click_down(input::mouse_buttons::left))
		{
			POINT cur;
			GetCursorPos(&cur);
			int dx = cur.x - drag_start_cursor.x;
			int dy = cur.y - drag_start_cursor.y;

			if (g_ctx->m_hwnd) {
				SetWindowPos(g_ctx->m_hwnd, nullptr,
					drag_start_rect.left + dx,
					drag_start_rect.top + dy,
					0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}
		else if (g_ctx->m_dragging && !g_input->click_down(input::mouse_buttons::left))
		{
			g_ctx->m_dragging = false;
		}

		this->m_obj_tab->update_input(this->m_pos, this->m_size);
	}

	std::shared_ptr<c_child> c_window::build_child(std::string name, child_width width, float y, std::function<void(c_child* ptr)> callback)
	{
		auto child = std::make_shared<c_child>(name, width, y);
		{
			if (!child)
			{
				slog::log::error("failed to create child: {}", name);
				return nullptr;
			}

			child->m_pos = child->m_relative_pos + this->m_pos + subtab_padding();
			this->m_childrens.push_back(child);
			callback(child.get());

			slog::log::success("[framework::c_window] created child: {}", name);
		}
		return child;
	}

	std::shared_ptr<c_tab> c_window::prebuild_tabs(std::function<void(c_tab* ptr)> callback)
	{
		auto obj = std::make_shared<c_tab>(this->m_pos, this->m_size);
		{
			slog::log::success("[+] prebuilded tab pointer");
			callback(obj.get());
			this->m_obj_tab = obj;
		}
		return obj;
	}

	void c_window::finish_tab_prebuild()
	{
		g_ctx->m_cur_tab = g_ctx->m_tabs[0].m_name;

		if (g_ctx->m_tabs[0].m_subtab.empty())
		{
			g_ctx->m_tabs[0].m_cur_subtab = "";
		}
	}

	math::c_vector_2d c_window::subtab_padding()
	{
		return math::c_vector_2d(15, 50);
	}

	math::c_vector_2d c_window::child_padding()
	{
		return math::c_vector_2d(20, 55);
	}
}