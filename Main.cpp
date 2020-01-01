//

#include "Project.hpp"

#include <iostream>
using namespace std;

int main( int argc, char **argv )
{
	if (argc > 1) {
		std::string luaSceneFile(argv[1]);
		std::string title("** Assignment 3 - [");
		title += luaSceneFile;
		title += "]";

		Window::launch(argc, argv, new Project(luaSceneFile), 1024, 768, title);

	} else {
		cout << "Must supply Lua file as First argument to program.\n";
        cout << "For example:\n";
        cout << "./Project Assets/simpleScene.lua\n";
	}

	return 0;
}
