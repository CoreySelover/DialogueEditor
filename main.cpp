#include <iostream>
#include <fstream>

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

#include "json.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// File picker
// ---------------------------------------------------------------------------

std::string openFilePicker(const std::string& mode = "open")
{
    char buf[MAX_PATH] = {};

    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "JSON Files\0*.json\0All Files\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = (mode == "save") ? "Save Dialogue File" : "Open Dialogue File";

    if (mode == "save")
    {
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        ofn.lpstrDefExt = "json";
        if (!GetSaveFileNameA(&ofn)) return "";
    }
    else
    {
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (!GetOpenFileNameA(&ofn)) return "";
    }

    return std::string(buf);
}

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

sf::RenderWindow window(sf::VideoMode({ 1920, 1080 }), "Dialogue Editor", sf::State::Windowed);
tgui::Gui gui{ window };

std::string fileName = "";

json g_dialogueData;

std::vector<std::string> dialogueIds;

std::string currentId = "";


json* getJsonNodeFromPath(json& root, const std::vector<tgui::String>& path)
{
    if (!root.contains("dialogues") || path.empty())
        return nullptr;

    json* current = nullptr;

    for (auto& dialogue : root["dialogues"])
    {
        if (dialogue.value("id", "") == path[0].toStdString())
        {
            current = &dialogue;
            break;
        }
    }

    if (!current) return nullptr;

    for (size_t i = 1; i < path.size(); i++)
    {
        std::string key = path[i].toStdString();
        bool found = false;

        for (const char* arr : { "texts", "choices" })
        {
            if (!current->contains(arr)) continue;
            for (auto& item : (*current)[arr])
            {
                if (item.value("id", "") == key)
                {
                    current = &item;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }

        if (!found) return nullptr;
    }

    return current;
}

std::string determineNodeType(const std::vector<tgui::String>& path)
{
    json* node = getJsonNodeFromPath(g_dialogueData, path);
    if (!node) return "";

    if (path.size() == 1)        return "Dialogue";
    if (node->contains("portrait")) return "Text";
    return "Choice";
}

void processJsonNode(const json& node, std::vector<tgui::String> path, tgui::TreeView::Ptr tree)
{
    if (!node.contains("id")) return;

    std::string label = node.value("id", "");
    path.push_back(label);
    tree->addItem(path);

    if (node.contains("texts"))
        for (const auto& t : node["texts"])
            processJsonNode(t, path, tree);

    if (node.contains("choices"))
        for (const auto& c : node["choices"])
            processJsonNode(c, path, tree);
}

void loadFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + filename);

    file >> g_dialogueData;

    auto tree = gui.get<tgui::TreeView>("DialogueTree");
    tree->removeAllItems();

    for (const auto& dialogue : g_dialogueData["dialogues"])
    {
        std::vector<tgui::String> context;
        processJsonNode(dialogue, context, tree);
    }

    tree->collapseAll();
}

void saveFile()
{
    std::string chosen = openFilePicker("save");
    if (chosen.empty()) return;
    fileName = chosen;
    std::ofstream file(fileName);
    file << g_dialogueData.dump(4);
}

void onButtonPress(const std::string& idPrefix, const std::string& portrait, const std::string& text)
{
    auto tree = gui.get<tgui::TreeView>("DialogueTree");
    auto path = tree->getSelectedItem();
    if (path.empty()) return;

    json* selected = getJsonNodeFromPath(g_dialogueData, path);
    if (!selected) return;

    std::string type = determineNodeType(path);

    std::vector<tgui::String> parentPath = path;
    json* parent = nullptr;

    if (type == "Text")
    {
        parentPath.pop_back();
        parent = parentPath.empty()
            ? nullptr
            : getJsonNodeFromPath(g_dialogueData, parentPath);
    }
    else
    {
        parent = selected;
    }

    if (!parent) return;

    if (!parent->contains("texts"))
        (*parent)["texts"] = json::array();

    std::string dialogueName = path[0].toStdString();
    int count = (int)(*parent)["texts"].size();
    std::string id = dialogueName + "_" + idPrefix + "_" + std::to_string(count);

    json newNode = {
        {"id",      id},
        {"portrait",portrait},
        {"text",    text}
    };

    (*parent)["texts"].push_back(newNode);

    parentPath.push_back(id);
    tree->addItem(parentPath);
    tree->selectItem(parentPath);
}

int main()
{
    try { gui.loadWidgetsFromFile("editor.txt"); }
    catch (const tgui::Exception& e) { std::cout << e.what() << std::endl; }

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

        gui.get<tgui::Button>("SaveButton")->setEnabled(true);
        gui.get<tgui::Button>("ResetButton")->setEnabled(true);
        });

    gui.get<tgui::Button>("SaveButton")->onPress([]() {

        auto tree = gui.get<tgui::TreeView>("DialogueTree");
        auto path = tree->getSelectedItem();
        if (path.empty()) return;

        json* node = getJsonNodeFromPath(g_dialogueData, path);
        if (!node) return;

        std::string type = determineNodeType(path);

        if (type == "Text")
        {
            std::string newId = gui.get<tgui::EditBox>("IDEditBox")->getText().toStdString();
            (*node)["id"] = newId;
            (*node)["portrait"] = gui.get<tgui::EditBox>("PortraitEditBox")->getText().toStdString();
            (*node)["text"] = gui.get<tgui::TextArea>("TextArea")->getText().toStdString();
            tree->changeItem(path, newId);
        }
        else if (type == "Choice")
        {
            std::string newText = gui.get<tgui::TextArea>("TextArea")->getText().toStdString();
            (*node)["text"] = newText;
            tree->changeItem(path, newText);
        }
        else if (type == "Dialogue")
        {
            std::string newId = gui.get<tgui::EditBox>("IDEditBox")->getText().toStdString();
            (*node)["id"] = newId;
            tree->changeItem(path, newId);
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

        std::string id = "NewDialogue" + std::to_string(std::rand() % 100000);

        json newDialogue;
        newDialogue["id"] = id;
        newDialogue["texts"] = json::array();

        g_dialogueData["dialogues"].push_back(newDialogue);

        tree->addItem({ id });
        tree->selectItem({ id });
        });

    gui.get<tgui::Button>("AddTextButton")->onPress([]() {
        onButtonPress("NEW_TEXT", "", "");
        });

    gui.get<tgui::Button>("AddChoiceButton")->onPress([]() {

        auto tree = gui.get<tgui::TreeView>("DialogueTree");
        auto path = tree->getSelectedItem();
        if (path.empty()) return;

        std::string nodeType = determineNodeType(path);
        if (nodeType != "Text") return;

        json* parent = getJsonNodeFromPath(g_dialogueData, path);
        if (!parent) return;

        if (!parent->contains("choices"))
            (*parent)["choices"] = json::array();

        std::string id = "CHOICE_" + std::to_string(std::rand() % 100000);

        json newChoice = {
            {"id", id},
            {"text", "New Choice"},
            {"texts", json::array()}
        };

        (*parent)["choices"].push_back(newChoice);

        path.push_back(id);

        tree->addItem(path);
        tree->selectItem(path);
        });

    gui.get<tgui::Button>("DeleteButton")->onPress([]() {
        auto tree = gui.get<tgui::TreeView>("DialogueTree");
        auto path = tree->getSelectedItem();
        if (path.empty()) return;

        if (path.size() == 1)
        {
            auto& arr = g_dialogueData["dialogues"];
            arr.erase(std::remove_if(arr.begin(), arr.end(),
                [&](const json& d) { return d["id"] == path[0].toStdString(); }),
                arr.end());
        }
        else
        {
            std::vector<tgui::String> parentPath = path;
            parentPath.pop_back();

            json* parent = getJsonNodeFromPath(g_dialogueData, parentPath);
            if (!parent) return;

            std::string key = path.back().toStdString();

            auto removeFromArray = [&](json& arr) {
                arr.erase(std::remove_if(arr.begin(), arr.end(),
                    [&](const json& n) {
                        return (n.contains("id") && n["id"] == key) ||
                            (n.contains("text") && n["text"] == key);
                    }), arr.end());
                };

            if (parent->contains("texts"))    removeFromArray((*parent)["texts"]);
            if (parent->contains("choices"))  removeFromArray((*parent)["choices"]);
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
        auto path = tree->getSelectedItem();
        if (path.empty()) return;

        int index = tree->getItemIndexInParent(path);
        if (index <= 0) return;

        // Move in tree
        tree->setItemIndexInParent(path, index - 1);

        // Mirror in JSON
        if (path.size() == 1)
        {
            auto& arr = g_dialogueData["dialogues"];
            if (index < (int)arr.size())
                std::swap(arr[index], arr[index - 1]);
        }
        else
        {
            std::vector<tgui::String> parentPath = path;
            parentPath.pop_back();
            json* parent = getJsonNodeFromPath(g_dialogueData, parentPath);
            if (!parent) return;

            std::string id = path.back().toStdString();

            for (const char* key : { "texts", "choices" })
            {
                if (!parent->contains(key)) continue;
                auto& arr = (*parent)[key];
                for (int i = 0; i < (int)arr.size(); i++)
                {
                    if (arr[i].value("id", "") == id && i > 0)
                    {
                        std::swap(arr[i], arr[i - 1]);
                        break;
                    }
                }
            }
        }
        });

    gui.get<tgui::Button>("MoveDownButton")->onPress([]() {
        auto tree = gui.get<tgui::TreeView>("DialogueTree");
        auto path = tree->getSelectedItem();
        if (path.empty()) return;

        int index = tree->getItemIndexInParent(path);

        // Move in tree
        tree->setItemIndexInParent(path, index + 1);

        // Mirror in JSON
        if (path.size() == 1)
        {
            auto& arr = g_dialogueData["dialogues"];
            if (index + 1 < (int)arr.size())
                std::swap(arr[index], arr[index + 1]);
        }
        else
        {
            std::vector<tgui::String> parentPath = path;
            parentPath.pop_back();
            json* parent = getJsonNodeFromPath(g_dialogueData, parentPath);
            if (!parent) return;

            std::string id = path.back().toStdString();

            for (const char* key : { "texts", "choices" })
            {
                if (!parent->contains(key)) continue;
                auto& arr = (*parent)[key];
                for (int i = 0; i < (int)arr.size(); i++)
                {
                    if (arr[i].value("id", "") == id && i + 1 < (int)arr.size())
                    {
                        std::swap(arr[i], arr[i + 1]);
                        break;
                    }
                }
            }
        }
        });

    // -----------------------------------------------------------------------
    // Load / Save file buttons
    // -----------------------------------------------------------------------
    gui.get<tgui::Button>("LoadFileButton")->onPress([]() {
        std::string chosen = openFilePicker("open");
        if (chosen.empty()) return;
        fileName = chosen;
        g_dialogueData = {};
        try { loadFile(fileName); }
        catch (const std::exception& e) { std::cout << e.what() << std::endl; }
        });

    gui.get<tgui::Button>("SaveFileButton")->onPress(saveFile);

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                    window.close();

            gui.handleEvent(event.value());
        }

        window.clear();
        gui.draw();
        window.display();
    }

    return 0;
}