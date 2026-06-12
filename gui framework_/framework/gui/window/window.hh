#pragma once

namespace framework
{
	class c_window
	{
	public:
		c_window(std::string title, math::c_vector_2d pos, math::c_vector_2d size);
		~c_window() // no point in creating this in cc file as its only a destructor
		{
			slog::log::success("[+] window {} destroyed", m_title);
		}

		// no virtual as this is the base object
		void paint();
		void input();

		std::shared_ptr<c_child> build_child(std::string name, child_width width, float y, std::function<void(c_child* ptr)> callback);
		std::shared_ptr<c_tab> prebuild_tabs(std::function<void(c_tab* ptr)> callback);

		void finish_tab_prebuild();

		// child pading func
		math::c_vector_2d subtab_padding();
		math::c_vector_2d child_padding();
	private:
		// tab pointer
		std::shared_ptr<c_tab> m_obj_tab{};

		std::string m_title{};
		math::c_vector_2d m_pos{};
		math::c_vector_2d m_size{};

		// childrens
		std::vector<std::shared_ptr<c_child>> m_childrens{};
	};

}