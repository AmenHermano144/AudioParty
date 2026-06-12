#pragma once

namespace framework
{
	class c_dropdown : public c_base_element {
	public:
		c_dropdown(std::string label, int* val, std::vector<std::string> items, bool hide_label = false);

		void draw() override;
		void input() override;
		std::vector<std::string>& items() { return m_items; }

	private:
		std::string m_label{};
		int* m_val{};
		std::vector<std::string> m_items{};
	};
}