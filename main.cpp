#include <iostream>

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

#include "pugixml.hpp"

sf::RenderWindow window(sf::VideoMode(1920, 1080), "Dialogue Editor");
tgui::Gui gui{ window };

struct TextData {
	std::string text;
	std::string portrait;
};

std::unordered_map<std::string, TextData> textDataMap;
std::vector<std::string> dialogueIds;

void processNode(pugi::xml_node node, std::vector<tgui::String> context, tgui::TreeView::Ptr tree) {
    std::string nodeName = node.name();

    if (nodeName == "Dialogue") {
        std::string dId = node.attribute("id").as_string();
        dialogueIds.push_back(dId);
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
        std::string text = node.text().as_string();

        textDataMap[id] = { text, portrait };
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
        // start at Dialogues node.
        pugi::xml_node dialogues = doc.child("Dialogues");

        for (pugi::xml_node dialogue = dialogues.child("Dialogue"); dialogue; dialogue = dialogue.next_sibling("Dialogue")) {
            std::vector<tgui::String> context;
            processNode(dialogue, context, tree);
        }

        tree->collapseAll();
    }
	else {
		throw std::runtime_error("Failed to load file: " + filename);
	}
}

void processTreeNode(const tgui::TreeView::ConstNode treeNode, pugi::xml_node& xmlParentNode) {
    std::string nodeName = treeNode.text.toStdString();
    std::string nodeType = "";

    if (std::find(dialogueIds.begin(), dialogueIds.end(), nodeName) != dialogueIds.end()) {
        nodeType = "Dialogue";
	}
	else {
		if (nodeName.find("_") != std::string::npos) {
			nodeType = "Text";
		}
		else {
			nodeType = "Choice";
		}
	}

    if (nodeType == "Dialogue") {
        pugi::xml_node dialogueNode = xmlParentNode.append_child("Dialogue");
        dialogueNode.append_attribute("id") = nodeName.c_str();
        for (const auto& child : treeNode.nodes) {
            processTreeNode(child, dialogueNode);
        }
    }
    else if (nodeType == "Text") {
        pugi::xml_node textNode = xmlParentNode.append_child("Text");
        textNode.append_attribute("id") = nodeName.c_str();
        textNode.append_attribute("portrait") = textDataMap[nodeName].portrait.c_str();
        textNode.append_child(pugi::node_pcdata).set_value(textDataMap[nodeName].text.c_str());

        for (const auto& child : treeNode.nodes) {
            processTreeNode(child, textNode);
        }
    }
    else if (nodeType == "Choice") {
        pugi::xml_node choiceNode = xmlParentNode.append_child("Choice");
        choiceNode.append_child(pugi::node_pcdata).set_value(nodeName.c_str());

        // Add line break back in if this node has children.
        if (!treeNode.nodes.empty()) {
			choiceNode.append_child(pugi::node_pcdata).set_value("\n");
		}
        for (const auto& child : treeNode.nodes) {
            processTreeNode(child, choiceNode);
        }
    }
}

void saveFile()
{
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("Dialogues");

    auto tree = gui.get<tgui::TreeView>("DialogueTree");

    for (const auto& treeNode : tree->getNodes()) {
        processTreeNode(treeNode, root);
    }

    doc.save_file("../CrescentTerminal/CrescentTerminal/Assets/Data/TEST.xml");
}

int main()
{
    gui.loadWidgetsFromFile("editor.txt");

    // Callbacks
    gui.get<tgui::TreeView>("DialogueTree")->onItemSelect([](const std::vector<tgui::String>& selectedItem) {
        if (selectedItem.empty()) {
            gui.get<tgui::Label>("IDLabel")->setText("Text ID:");
            gui.get<tgui::EditBox>("PortraitEditBox")->setText("");
            gui.get<tgui::TextArea>("TextArea")->setText(gui.get<tgui::TextArea>("TextArea")->getDefaultText());
            gui.get<tgui::Button>("SaveButton")->setEnabled(false);
            gui.get<tgui::Button>("ResetButton")->setEnabled(false);
        }    
        else {
            std::string id = selectedItem.back().toStdString();
			if (textDataMap.find(id) != textDataMap.end()) {
				gui.get<tgui::Label>("IDLabel")->setText("Text ID: " + id);
				gui.get<tgui::EditBox>("PortraitEditBox")->setText(textDataMap[id].portrait);
				gui.get<tgui::TextArea>("TextArea")->setText(textDataMap[id].text);
				gui.get<tgui::Button>("SaveButton")->setEnabled(true);
				gui.get<tgui::Button>("ResetButton")->setEnabled(true);
			}
        }
    });

    gui.get<tgui::Button>("SaveButton")->onPress([]() {
		std::string id = gui.get<tgui::Label>("IDLabel")->getText().toStdString();
		id = id.substr(id.find(":") + 2);
		textDataMap[id].portrait = gui.get<tgui::EditBox>("PortraitEditBox")->getText().toStdString();
		textDataMap[id].text = gui.get<tgui::TextArea>("TextArea")->getText().toStdString();
	});

    gui.get<tgui::Button>("ResetButton")->onPress([]() {
        std::string id = gui.get<tgui::Label>("IDLabel")->getText().toStdString();
        id = id.substr(id.find(":") + 2);
        gui.get<tgui::EditBox>("PortraitEditBox")->setText(textDataMap[id].portrait);
        gui.get<tgui::TextArea>("TextArea")->setText(textDataMap[id].text);
    });

    gui.get<tgui::Button>("SaveFileButton")->onPress(saveFile);

    /*tgui::FileDialog::Ptr filePicker = tgui::FileDialog::create("Open file", "Open", true);
    filePicker->setFileTypeFilters({
         {"XML files", {"*.xml"}}
        });
    filePicker->setPosition("50%", "50%");
    filePicker->setOrigin(0.5f, 0.5f);
    filePicker->setClientSize({ 1080, 720 });
    gui.add(filePicker, "filePicker");*/

    tgui::Filesystem::Path fileName;
    std::string fileName2 = "../CrescentTerminal/CrescentTerminal/Assets/Data/TEST.xml"; // DEBUG
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