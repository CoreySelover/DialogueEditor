#include <iostream>

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

//#include "pugixml.hpp"
#include "json.hpp"
#include <fstream>

using json = nlohmann::json;

sf::RenderWindow window(sf::VideoMode({ 1920, 1080 }), "Dialogue Editor", sf::State::Windowed);
tgui::Gui gui{ window };

// While building in VS
std::string fileName2 = "../CrescentTerminal/CrescentTerminal/Assets/Data/Dialogue.json";

// While running the exe
//std::string fileName2 = "../../../CrescentTerminal/CrescentTerminal/Assets/Data/Dialogue.xml";

json g_dialogueData;

std::vector<std::string> dialogueIds;

std::string currentId = "";

////////////////////////////////////////////////////////////////////////////////////////////////////
// XML to JSON conversion
////////////////////////////////////////////////////////////////////////////////////////////////////

//// Forward declaration
//json parseTextNode(const pugi::xml_node& node);
//
//// Parse <Choice>
//json parseChoiceNode(const pugi::xml_node& choiceNode)
//{
//    json choiceJson;
//
//    // Choice text (everything before child nodes)
//    choiceJson["text"] = choiceNode.child_value();
//
//    // Children (Text nodes inside this choice)
//    json children = json::array();
//    for (pugi::xml_node child : choiceNode.children())
//    {
//        if (std::string(child.name()) == "Text")
//        {
//            children.push_back(parseTextNode(child));
//        }
//    }
//
//    if (!children.empty())
//        choiceJson["children"] = children;
//
//    return choiceJson;
//}
//
//// Parse <Text>
//json parseTextNode(const pugi::xml_node& node)
//{
//    json textJson;
//
//    textJson["id"] = node.attribute("id").as_string();
//    textJson["portrait"] = node.attribute("portrait").as_string();
//
//    // Base text (excluding nested nodes)
//    textJson["text"] = node.child_value();
//
//    // Choices
//    json choices = json::array();
//    for (pugi::xml_node child : node.children("Choice"))
//    {
//        choices.push_back(parseChoiceNode(child));
//    }
//
//    if (!choices.empty())
//        textJson["choices"] = choices;
//
//    return textJson;
//}
//
//void convertDialoguesToJson(std::string filepath)
//{
//    pugi::xml_document doc;
//    if (!doc.load_file(filepath.c_str()))
//    {
//        throw std::runtime_error("Failed to load XML");
//    }
//
//    json output;
//    output["dialogues"] = json::array();
//
//    for (pugi::xml_node dialogue : doc.child("Dialogues").children("Dialogue"))
//    {
//        json dialogueJson;
//        dialogueJson["id"] = dialogue.attribute("id").as_string();
//
//        json texts = json::array();
//
//        for (pugi::xml_node text : dialogue.children("Text"))
//        {
//            texts.push_back(parseTextNode(text));
//        }
//
//        dialogueJson["texts"] = texts;
//        output["dialogues"].push_back(dialogueJson);
//    }
//
//    // Save JSON
//    std::ofstream file("dialogues.json");
//    file << output.dump(4); // pretty print
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// End of XML to JSON conversion
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

json* getJsonNodeFromPath(json& root, const std::vector<tgui::String>& path)
{
    json* current = nullptr;

    for (auto& dialogue : root["dialogues"])
    {
        if (dialogue["id"] == path[0].toStdString())
        {
            current = &dialogue;
            break;
        }
    }

    if (!current) return nullptr;

    for (size_t i = 1; i < path.size(); i++)
    {
        std::string key = path[i].toStdString();

        if (current->contains("texts"))
        {
            for (auto& t : (*current)["texts"])
            {
                if (t["id"] == key)
                {
                    current = &t;
                    goto next;
                }
            }
        }

        if (current->contains("choices"))
        {
            for (auto& c : (*current)["choices"])
            {
                if (c["text"] == key)
                {
                    current = &c;
                    goto next;
                }
            }
        }

        if (current->contains("children"))
        {
            for (auto& c : (*current)["children"])
            {
                if (c["id"] == key || c["text"] == key)
                {
                    current = &c;
                    goto next;
                }
            }
        }

        return nullptr;

    next:;
    }

    return current;
}

std::string determineNodeType(const std::vector<tgui::String>& path)
{
    json* node = getJsonNodeFromPath(g_dialogueData, path);
    if (!node) return "";

    if (node->contains("texts")) return "Dialogue";
    if (node->contains("id")) return "Text";
    return "Choice";
}

void processJsonNode(const json& node, std::vector<tgui::String> context, tgui::TreeView::Ptr tree)
{
    if (node.contains("id") && node.contains("texts"))
    {
        // Dialogue
        std::string id = node["id"];
        dialogueIds.push_back(id);
        context.push_back(id);
        tree->addItem(context);

        for (const auto& text : node["texts"])
            processJsonNode(text, context, tree);
    }
    else if (node.contains("id") && node.contains("text"))
    {
        std::string id = node["id"];
        context.push_back(id);
        tree->addItem(context);

        if (node.contains("choices"))
        {
            for (const auto& choice : node["choices"])
                processJsonNode(choice, context, tree);
        }
    }
    else if (node.contains("text") && !node.contains("id"))
    {
        // Choice
        std::string choiceText = node["text"];
        context.push_back(choiceText);
        tree->addItem(context);

        if (node.contains("children"))
        {
            for (const auto& child : node["children"])
                processJsonNode(child, context, tree);
        }
    }
}

void loadFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + filename);
    }

    file >> g_dialogueData;

    auto tree = gui.get<tgui::TreeView>("DialogueTree");

    for (const auto& dialogue : g_dialogueData["dialogues"])
    {
        std::vector<tgui::String> context;
        processJsonNode(dialogue, context, tree);
    }

    tree->collapseAll();
}

void saveFile()
{
    std::ofstream file(fileName2);
    file << g_dialogueData.dump(4);
}

void onButtonPress(const std::string& idPrefix, const std::string& portrait, const std::string& text)
{
    auto tree = gui.get<tgui::TreeView>("DialogueTree");
    auto path = tree->getSelectedItem();
    if (path.empty()) return;

    json* parent = getJsonNodeFromPath(g_dialogueData, path);
    if (!parent) return;

    std::string type = determineNodeType(path);

    // If clicking on a Text node, insert under its parent
    if (type == "Text")
    {
        path.pop_back();
        parent = getJsonNodeFromPath(g_dialogueData, path);
    }

    std::string newId = idPrefix + std::to_string(std::rand() % 100000);

    json newNode;
    newNode["id"] = newId;
    newNode["portrait"] = portrait;
    newNode["text"] = text;

    if (!parent->contains("texts"))
        (*parent)["texts"] = json::array();

    (*parent)["texts"].push_back(newNode);

    // Update UI
    path.push_back(newId);
    tree->addItem(path);
    tree->selectItem(path);
}

int main()
{
    try { gui.loadWidgetsFromFile("editor.txt"); }
	catch (const tgui::Exception& e) { std::cout << e.what() << std::endl;}

    //convertDialoguesToJson(fileName2);

	gui.setTextSize(24);

	auto tree = gui.get<tgui::TreeView>("DialogueTree");
	tree->getVerticalScrollbar()->setPolicy(tgui::Scrollbar::Policy::Automatic);

    // Callbacks
    tree->onItemSelect([](const std::vector<tgui::String>& selectedItem) {

        if (selectedItem.empty())
            return;

        json* node = getJsonNodeFromPath(g_dialogueData, selectedItem);
        if (!node) return;

        currentId = selectedItem.back().toStdString();

        auto idBox = gui.get<tgui::EditBox>("IDEditBox");
        auto portraitBox = gui.get<tgui::EditBox>("PortraitEditBox");
        auto textArea = gui.get<tgui::TextArea>("TextArea");

        std::string type = determineNodeType(selectedItem);

        if (type == "Dialogue")
        {
            idBox->setText((*node)["id"].get<std::string>());
            portraitBox->setText("");
            textArea->setText("");
        }
        else if (type == "Text")
        {
            idBox->setText((*node)["id"].get<std::string>());
            portraitBox->setText(node->value("portrait", ""));
            textArea->setText(node->value("text", ""));
        }
        else // Choice
        {
            idBox->setText("");
            portraitBox->setText("");
            textArea->setText(node->value("text", ""));
        }
        });

    gui.get<tgui::Button>("SaveButton")->onPress([]() {

        auto tree = gui.get<tgui::TreeView>("DialogueTree");
        auto selectedItem = tree->getSelectedItem();
        if (selectedItem.empty()) return;

        json* node = getJsonNodeFromPath(g_dialogueData, selectedItem);
        if (!node) return;

        std::string type = determineNodeType(selectedItem);

        if (type == "Text")
        {
            (*node)["portrait"] = gui.get<tgui::EditBox>("PortraitEditBox")->getText().toStdString();
            (*node)["text"] = gui.get<tgui::TextArea>("TextArea")->getText().toStdString();
        }
        else if (type == "Choice")
        {
            (*node)["text"] = gui.get<tgui::TextArea>("TextArea")->getText().toStdString();
        }
        else if (type == "Dialogue")
        {
            (*node)["id"] = gui.get<tgui::EditBox>("IDEditBox")->getText().toStdString();
        }
        });

    gui.get<tgui::Button>("ResetButton")->onPress([]() {
        auto tree = gui.get<tgui::TreeView>("DialogueTree");
        auto selectedItem = tree->getSelectedItem();
        if (selectedItem.empty()) return;

        json* node = getJsonNodeFromPath(g_dialogueData, selectedItem);
        if (!node) return;

        gui.get<tgui::EditBox>("PortraitEditBox")->setText(node->value("portrait", ""));
        gui.get<tgui::TextArea>("TextArea")->setText(node->value("text", ""));
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
        auto path = tree->getSelectedItem();
        if (path.empty()) return;

        if (path.size() == 1)
        {
            // Remove dialogue
            auto& arr = g_dialogueData["dialogues"];
            arr.erase(std::remove_if(arr.begin(), arr.end(),
                [&](const json& d) { return d["id"] == path[0].toStdString(); }),
                arr.end());
        }
        else
        {
            // Remove from parent
            std::vector<tgui::String> parentPath = path;
            parentPath.pop_back();

            json* parent = getJsonNodeFromPath(g_dialogueData, parentPath);
            if (!parent) return;

            std::string key = path.back().toStdString();

            auto removeFromArray = [&](json& arr)
                {
                    arr.erase(std::remove_if(arr.begin(), arr.end(),
                        [&](const json& n) {
                            return (n.contains("id") && n["id"] == key) ||
                                (n.contains("text") && n["text"] == key);
                        }),
                        arr.end());
                };

            if (parent->contains("texts")) removeFromArray((*parent)["texts"]);
            if (parent->contains("choices")) removeFromArray((*parent)["choices"]);
            if (parent->contains("children")) removeFromArray((*parent)["children"]);
        }

        tree->removeItem(path, false);
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
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                    window.close();
            }

            gui.handleEvent(event.value());
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