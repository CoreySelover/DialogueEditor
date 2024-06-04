#include <iostream>

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

#include "pugixml.hpp"

void loadFile(std::string filename)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.c_str());

	if (result)
	{
		std::cout << "XML file loaded successfully" << std::endl;
	}
	else
	{
		std::cout << "XML file loading failed" << std::endl;
	}
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Dialogue Editor");
    tgui::Gui gui{ window };
    gui.loadWidgetsFromFile("editor.txt");

    tgui::FileDialog::Ptr filePicker = tgui::FileDialog::create("Open file", "Open", true);
    filePicker->setFileTypeFilters({
         {"XML files", {"*.xml"}}
        });
    filePicker->setPosition("50%", "50%");
    filePicker->setOrigin(0.5f, 0.5f);
    filePicker->setClientSize({ 1080, 720 });
    gui.add(filePicker, "filePicker");

    tgui::Filesystem::Path fileName;
    std::string fileName2 = "../CrescentTerminal/CrescentTerminal/Assets/Data/dialogue.xml"; // DEBUG
    bool loaded = false;


    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            gui.handleEvent(event);

            if (event.type == sf::Event::Closed)
                window.close();
        }

        if (!loaded) {
            loadFile(fileName2);
            loaded = true;
        }

        window.clear();

        gui.draw();

        window.display();
    }

    return 0;
}