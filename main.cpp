#include <iostream>

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

#include "pugixml.hpp"

sf::RenderWindow window(sf::VideoMode(1920, 1080), "Dialogue Editor");
tgui::Gui gui{ window };

void processNode(pugi::xml_node node, std::vector<tgui::String> context, tgui::TreeView::Ptr tree) {
    std::string nodeName = node.name();

    if (nodeName == "Dialogue") {
        std::string dId = node.attribute("id").as_string();
        context.push_back(dId);
        tree->addItem(context);

        // Recursively process child nodes
        for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
            processNode(child, context, tree);
        }

    }
    else if (nodeName == "Text") {
        std::string id = node.attribute("id").as_string();
        std::string portrait = node.attribute("portrait").as_string();
        context.push_back(id);
        tree->addItem(context);

        // Recursively process child nodes
        for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
            processNode(child, context, tree);
        }

    }
    else if (nodeName == "Choice") {
        std::string choiceText = node.text().as_string();
        // Remove line breaks
        choiceText.erase(std::remove(choiceText.begin(), choiceText.end(), '\n'), choiceText.end());
        context.push_back(choiceText);
        tree->addItem(context);

        // Recursively process child nodes
        for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
            processNode(child, context, tree);
        }
    }
}


void loadFile(std::string filename)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.c_str());

    auto tree = gui.get<tgui::TreeView>("DialogueTree");

    if (result) {
        // load each dialogue
        for (pugi::xml_node dialogue = doc.child("Dialogue"); dialogue; dialogue = dialogue.next_sibling("Dialogue")) {
            std::vector<tgui::String> context;
            processNode(dialogue, context, tree);
        }

        tree->collapseAll();
    }
	else {
		throw std::runtime_error("Failed to load file: " + filename);
	}
}

int main()
{
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