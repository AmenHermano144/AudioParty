#pragma once

namespace framework
{
	class c_listbox : public c_base_element
	{
	public:
		c_listbox(std::string label, int* var, std::vector<std::string> items, float height, bool hide_label = false);

		void draw() override;
		void input() override;
	private:
		int* m_var{};
		std::vector<std::string> m_items{};
		float m_height{};

		bool m_listbox_item_hovered{false};
		float m_scroll_offset{ 0.f };
		float m_scroll_target{ 0.f };
	};
}