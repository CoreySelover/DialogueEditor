#include <iostream>

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

#include "pugixml.hpp"

sf::RenderWindow window(sf::VideoMode(1920, 1080), "Dialogue Editor");
tgui::Gui gui{ window };

// While building in VS
//std::string fileName2 = "../CrescentTerminal/CrescentTerminal/Assets/Data/Dialogue.xml";

// While running the exe
std::string fileName2 = "../../../CrescentTerminal/CrescentTerminal/Assets/Data/Dialogue.xml";

struct TextData {
	std::string text;
	std::string portrait;
};

std::unordered_map<std::string, TextData> textDataMap;
std::vector<std::string> dialogueIds;

std::string currentId = "";

std::string determineNodeType(const std::vector<tgui::String>& selectedItem) {
    if (std::find(dialogueIds.begin(), dialogueIds.end(), selectedItem.back()) != dialogueIds.end()) {
        return "Dialogue";
    }
    else {
        if (textDataMap.count(selectedItem.back().toStdString()) != 0) {
            return "Text";
        }
        else {
            return "Choice";
        }
    }
}

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
        pugi::xml_node dialogues = doc.child("Dialogues");

        for (pugi::xml_node dialogue = dialogues.child("Dialogue"); dialogue; dialogue = dialogue.next_sibling("Dialogue")) {
            std::vector<tgui::String> context;
            processNode(dialogue, context, tree);
        }

        tree->collapseAll();
        std::cout << "Loaded successfully." << std::endl;
    }
	else {
		std::cout << "Error loading file: " << result.description() << std::endl;
	}
}

void processTreeNode(const tgui::TreeView::ConstNode treeNode, pugi::xml_node& xmlParentNode) {
    std::string nodeName = treeNode.text.toStdString();
    std::string nodeType = determineNodeType({ treeNode.text });

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

    doc.save_file(fileName2.c_str());
}

void onButtonPress(const std::string& idPrefix, const std::string& portrait, const std::string& text) {
    auto tree = gui.get<tgui::TreeView>("DialogueTree");
    auto context = tree->getSelectedItem();
    if (context.empty()) return;

    int index = tree->getItemIndexInParent(context);

    // Determine parent type
    std::string parentType = determineNodeType(context);
    if (parentType == "Text") {
        context.pop_back();
    }

    // Create a unique ID
	std::string prefix = idPrefix;
    if (prefix == "NEW_TEXT") {
		prefix = context.front().toStdString() + "_";
    }
    std::string id = prefix + std::to_string(textDataMap.size());
    if (textDataMap.find(id) != textDataMap.end()) {
        int i = 1;
        while (textDataMap.find(id + "_" + std::to_string(i)) != textDataMap.end()) {
            i++;
        }
        id = id + "_" + std::to_string(i);
    }

    // Add to textDataMap and update tree
    textDataMap[id] = { text, portrait };
    context.push_back(id);
    tree->addItem(context);
    tree->setItemIndexInParent(context, index + 1);
    tree->selectItem(context);
    tree->getVerticalScrollbar()->setValue(tree->getVerticalScrollbar()->getMaxValue());
}

int main()
{
    try { gui.loadWidgetsFromFile("editor.txt"); }
	catch (const tgui::Exception& e) { std::cout << e.what() << std::endl;}

    // Callbacks
    gui.get<tgui::TreeView>("DialogueTree")->onItemSelect([](const std::vector<tgui::String>& selectedItem) {
        // Save previously selected item.
		std::string oldId = gui.get<tgui::EditBox>("IDEditBox")->getText().toStdString();
		if (oldId != "") {
            if (textDataMap.find(oldId) != textDataMap.end()) {
				textDataMap[oldId].portrait = gui.get<tgui::EditBox>("PortraitEditBox")->getText().toStdString();
				textDataMap[oldId].text = gui.get<tgui::TextArea>("TextArea")->getText().toStdString();
			}
			else {
				// Do nothing
			}
		}
        if (selectedItem.empty()) {
            gui.get<tgui::EditBox>("IDEditBox")->setText("");
            gui.get<tgui::EditBox>("PortraitEditBox")->setText("");
            gui.get<tgui::TextArea>("TextArea")->setText(gui.get<tgui::TextArea>("TextArea")->getDefaultText());
            gui.get<tgui::Button>("SaveButton")->setEnabled(false);
            gui.get<tgui::Button>("ResetButton")->setEnabled(false);
            gui.get<tgui::Button>("DeleteButton")->setEnabled(false);
        }
        else {
            std::string id = selectedItem.back().toStdString();
            currentId = id;
            gui.get<tgui::Button>("DeleteButton")->setEnabled(true);
            if (textDataMap.find(id) != textDataMap.end()) {
                gui.get<tgui::EditBox>("IDEditBox")->setText(id);
                gui.get<tgui::EditBox>("IDEditBox")->setEnabled(true);
                gui.get<tgui::EditBox>("PortraitEditBox")->setText(textDataMap[id].portrait);
                gui.get<tgui::EditBox>("PortraitEditBox")->setEnabled(true);
                gui.get<tgui::TextArea>("TextArea")->setText(textDataMap[id].text);
                gui.get<tgui::TextArea>("TextArea")->setEnabled(true);
                gui.get<tgui::Button>("SaveButton")->setEnabled(true);
                gui.get<tgui::Button>("ResetButton")->setEnabled(true);
            }
            else if (dialogueIds.size() > 0 && std::find(dialogueIds.begin(), dialogueIds.end(), id) != dialogueIds.end()) {
                gui.get<tgui::EditBox>("IDEditBox")->setText(id);
                gui.get<tgui::EditBox>("IDEditBox")->setEnabled(true);
                gui.get<tgui::EditBox>("PortraitEditBox")->setText("");
                gui.get<tgui::EditBox>("PortraitEditBox")->setEnabled(false);
                gui.get<tgui::TextArea>("TextArea")->setText("");
                gui.get<tgui::TextArea>("TextArea")->setEnabled(false);
                gui.get<tgui::Button>("SaveButton")->setEnabled(true);
                gui.get<tgui::Button>("ResetButton")->setEnabled(false);
            }
            else {
                gui.get<tgui::EditBox>("IDEditBox")->setText("");
                gui.get<tgui::EditBox>("IDEditBox")->setEnabled(false);
                gui.get<tgui::EditBox>("PortraitEditBox")->setText("");
                gui.get<tgui::EditBox>("PortraitEditBox")->setEnabled(false);
                gui.get<tgui::TextArea>("TextArea")->setText(currentId);
                gui.get<tgui::TextArea>("TextArea")->setEnabled(true);
                gui.get<tgui::Button>("SaveButton")->setEnabled(true);
                gui.get<tgui::Button>("ResetButton")->setEnabled(true);
            }
        }
        });

    gui.get<tgui::Button>("SaveButton")->onPress([]() {
        auto tree = gui.get<tgui::TreeView>("DialogueTree");
        auto selectedItem = tree->getSelectedItem();
        if (selectedItem.empty()) return;
        std::string nodeType = determineNodeType(selectedItem);
        std::string newId = gui.get<tgui::EditBox>("IDEditBox")->getText().toStdString();

        if (nodeType == "Text" && textDataMap.count(newId) != 0) {
            // Update the text
            textDataMap[newId].portrait = gui.get<tgui::EditBox>("PortraitEditBox")->getText().toStdString();
            textDataMap[newId].text = gui.get<tgui::TextArea>("TextArea")->getText().toStdString();
        }
        else if (nodeType == "Text" && currentId != newId) {
            // Rename the text
			textDataMap[newId] = textDataMap[currentId];
			textDataMap.erase(currentId);
            textDataMap[newId].portrait = gui.get<tgui::EditBox>("PortraitEditBox")->getText().toStdString();
            textDataMap[newId].text = gui.get<tgui::TextArea>("TextArea")->getText().toStdString();
            currentId = newId;
            tree->changeItem(selectedItem, newId);
        }
        else if (nodeType == "Dialogue") {
            // Rename the dialogue
            dialogueIds.erase(std::remove(dialogueIds.begin(), dialogueIds.end(), currentId), dialogueIds.end());
            dialogueIds.push_back(newId);
            currentId = newId;
            tree->changeItem(selectedItem, newId);
        }
        else if (nodeType == "Choice") {
            tree->changeItem(selectedItem, gui.get<tgui::TextArea>("TextArea")->getText().toStdString());
        }
    });

    gui.get<tgui::Button>("ResetButton")->onPress([]() {
        std::string id = gui.get<tgui::EditBox>("IDEditBox")->getText().toStdString();
        gui.get<tgui::EditBox>("PortraitEditBox")->setText(textDataMap[id].portrait);
        gui.get<tgui::TextArea>("TextArea")->setText(textDataMap[id].text);
    });

    gui.get<tgui::Button>("AddDialogueButton")->onPress([]() {
		auto tree = gui.get<tgui::TreeView>("DialogueTree");
        std::string dialogueId = "New Dialogue" + std::to_string(dialogueIds.size());
        // check to make sure the id is unique
        int i = 1;
        while (std::find(dialogueIds.begin(), dialogueIds.end(), dialogueId) != dialogueIds.end()) {
			dialogueId = "New Dialogue" + std::to_string(dialogueIds.size() + i);
			i++;
		}
		tree->addItem({ dialogueId });
        dialogueIds.push_back(dialogueId);
        tree->selectItem({ dialogueId });
        tree->getVerticalScrollbar()->setValue(tree->getVerticalScrollbar()->getMaxValue());
	});

    gui.get<tgui::Button>("AddTextButton")->onPress([]() {
		onButtonPress("NEW_TEXT", "", "");
    });

    gui.get<tgui::Button>("AddChoiceButton")->onPress([]() {

        auto tree = gui.get<tgui::TreeView>("DialogueTree");
        auto context = tree->getSelectedItem();
        if (context.empty()) return;

		int index = tree->getItemIndexInParent(context);

		std::string choiceText = "New Choice";
		// check to make sure the id is unique
		int i = 1;
		while (std::find(dialogueIds.begin(), dialogueIds.end(), choiceText) != dialogueIds.end()) {
			choiceText = "New Choice" + std::to_string(i);
			i++;
		}

		// This code is unique to choices so we can't use the onButtonPress function
        std::string nodeType = determineNodeType(context);
        if (nodeType != "Text") return;

		context.push_back(choiceText);
		tree->addItem(context);
		tree->setItemIndexInParent(context, index + 1);
		tree->selectItem(context);
        tree->getVerticalScrollbar()->setValue(tree->getVerticalScrollbar()->getMaxValue());

	});

    gui.get<tgui::Button>("DeleteButton")->onPress([]() {
        auto tree = gui.get<tgui::TreeView>("DialogueTree");
        auto selectedItem = tree->getSelectedItem();
        if (selectedItem.empty()) return;

        std::string nodeType = determineNodeType(selectedItem);
        if (nodeType == "Dialogue") {
            dialogueIds.erase(std::remove(dialogueIds.begin(), dialogueIds.end(), selectedItem.back().toStdString()), dialogueIds.end());
        }
        else if (nodeType == "Text") {
            textDataMap.erase(selectedItem.back().toStdString());
        }

        tree->removeItem(selectedItem, false);
    });

    gui.get<tgui::Button>("QUEUEButton")->onPress([]() {
		onButtonPress("QUEUE_EVENT", "QUEUE_EVENT", "");
    });

    gui.get<tgui::Button>("WAIT_SHORTButton")->onPress([]() {
		onButtonPress("WAIT_SHORT", "WAIT_SHORT", "WAIT_SHORT");
    });

    gui.get<tgui::Button>("WAIT_LONGButton")->onPress([]() {
		onButtonPress("WAIT_LONG", "WAIT_LONG", "WAIT_LONG");
    });

    gui.get<tgui::Button>("CHAR_DIRButton")->onPress([]() {
		onButtonPress("CHAR_DIR", "WHICH_CHAR", "DIRECTION");
    });

    gui.get<tgui::Button>("CHAR_MOVEButton")->onPress([]() {
		onButtonPress("CHAR_MOVE", "WHICH_CHAR", "board,tilePos");
    });

	gui.get<tgui::Button>("MoveUpButton")->onPress([]() {
		auto tree = gui.get<tgui::TreeView>("DialogueTree");
		auto selectedItem = tree->getSelectedItem();
		if (selectedItem.empty()) return;
        int index = tree->getItemIndexInParent(selectedItem);
        if (index > 0) // -1 if not found, 0 if already at top
            tree->setItemIndexInParent(selectedItem, index - 1);
	});

    gui.get<tgui::Button>("MoveDownButton")->onPress([]() {
        auto tree = gui.get<tgui::TreeView>("DialogueTree");
        auto selectedItem = tree->getSelectedItem();
        if (selectedItem.empty()) return;
        int index = tree->getItemIndexInParent(selectedItem);
        tree->setItemIndexInParent(selectedItem, index + 1);
    });

    gui.get<tgui::Button>("SaveFileButton")->onPress(saveFile);

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