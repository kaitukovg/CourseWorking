#include "JSONLoader.h"
#include "Rectangle.h"
#include "Text.h"
#include "Line.h"
#include "Polyline.h"
#include "InputField.h"
#include "Button.h"
#include "HistoryGraph.h"
#include "Image.h"
#include "logger.h"
#include <fstream>
#include <iostream>
#include <filesystem>

using json = nlohmann::json;

// Вспомогательная функция для преобразования JSON в цвет
sf::Color JSONLoader::jsonToColor(const json& colorJson) {
    if (colorJson.is_array() && colorJson.size() >= 3) {
        int r = colorJson[0].get<int>();
        int g = colorJson[1].get<int>();
        int b = colorJson[2].get<int>();
        int a = colorJson.size() >= 4 ? colorJson[3].get<int>() : 255;
        return sf::Color(r, g, b, a);
    }
    return sf::Color::White;
}

// Основные методы JSONLoader
std::vector<std::unique_ptr<VisualObject>> JSONLoader::loadFromFile(
    const std::string& filename, 
    VariableDatabase* db,
    sf::Font* font) {
    
    std::vector<std::unique_ptr<VisualObject>> objects;
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::error("Cannot open JSON file: " + filename);
        return objects;
    }
    
    try {
        json j;
        file >> j;
        
        if (j.contains("objects") && j["objects"].is_array()) {
            for (const auto& objJson : j["objects"]) {
                auto obj = createObject(objJson, db, font);
                if (obj) {
                    objects.push_back(std::move(obj));
                }
            }
        }
        Logger::info("Loaded " + std::to_string(objects.size()) + " objects from " + filename);
    } catch (const std::exception& e) {
        Logger::error("Error parsing JSON file: " + std::string(e.what()));
    }
    
    return objects;
}

std::unique_ptr<VisualObject> JSONLoader::createObject(
    const nlohmann::json& objJson,
    VariableDatabase* db,
    sf::Font* font) {
    
    std::string type = objJson.value("type", "");
    std::string name = objJson.value("name", "");
    float x = objJson.value("x", 0.0f);
    float y = objJson.value("y", 0.0f);
    
    if (type == "Rectangle") {
        float width = objJson.value("width", 100.0f);
        float height = objJson.value("height", 50.0f);
        std::string variable = objJson.value("variable", "");
        sf::Color color = jsonToColor(objJson.value("color", json::array({255, 255, 255})));
        
        auto rect = std::make_unique<Rectangle>(x, y, width, height, color, name, db, variable);
        
        // Добавляем условия, если есть
        if (objJson.contains("conditions") && objJson["conditions"].is_array()) {
            for (const auto& condJson : objJson["conditions"]) {
                double value = condJson.value("value", 0.0);
                sf::Color condColor = jsonToColor(condJson.value("color", json::array({255, 255, 255})));
                rect->addCondition(value, condColor);
            }
        }
        
        return rect;
    }
    else if (type == "Text") {
        std::string content = objJson.value("content", "");
        unsigned int fontSize = objJson.value("fontSize", 20);
        std::string variable = objJson.value("variable", "");
        std::string format = objJson.value("format", "");
        sf::Color color = jsonToColor(objJson.value("color", json::array({255, 255, 255})));
        
        return std::make_unique<Text>(x, y, content, font, fontSize, color, name, db, variable, format);
    }
    else if (type == "Line") {
        float x2 = objJson.value("x2", 0.0f);
        float y2 = objJson.value("y2", 0.0f);
        sf::Color color = jsonToColor(objJson.value("color", json::array({255, 255, 255})));
        
        return std::make_unique<Line>(x, y, x2, y2, color, name, db);
    }
    else if (type == "Polyline") {
        std::vector<sf::Vector2f> points;
        if (objJson.contains("points") && objJson["points"].is_array()) {
            for (const auto& pointJson : objJson["points"]) {
                if (pointJson.is_array() && pointJson.size() >= 2) {
                    float px = pointJson[0].get<float>();
                    float py = pointJson[1].get<float>();
                    points.push_back(sf::Vector2f(px, py));
                }
            }
        } else {
            // Если точек нет, используем текущую позицию
            points.push_back(sf::Vector2f(x, y));
        }
        
        sf::Color color = jsonToColor(objJson.value("color", json::array({255, 255, 255})));
        std::string variable = objJson.value("variable", "");
        
        return std::make_unique<Polyline>(points, color, name, db, variable);
    }
    else if (type == "InputField") {
        float width = objJson.value("width", 200.0f);
        float height = objJson.value("height", 30.0f);
        unsigned int fontSize = objJson.value("fontSize", 16);
        std::string variable = objJson.value("variable", "");
        
        return std::make_unique<InputField>(x, y, width, height, font, fontSize, name, db, variable);
    }
    else if (type == "Button") {
        float width = objJson.value("width", 100.0f);
        float height = objJson.value("height", 40.0f);
        std::string text = objJson.value("text", "");
        unsigned int fontSize = objJson.value("fontSize", 16);
        std::string variable = objJson.value("variable", "");
        sf::Color color = jsonToColor(objJson.value("color", json::array({200, 200, 200})));
        sf::Color textColor = jsonToColor(objJson.value("textColor", json::array({0, 0, 0})));
        
        // Создаем кнопку без обработчика
        auto button = std::make_unique<Button>(x, y, width, height, text, font, fontSize, 
                                               color, name, db, variable, nullptr, textColor);
        
        // Устанавливаем действие, если оно есть
        if (objJson.contains("action")) {
            std::string action = objJson["action"];
            button->setAction(action, db);
        }
        
        return button;
    }
    else if (type == "HistoryGraph") {
        float width = objJson.value("width", 400.0f);
        float height = objJson.value("height", 200.0f);
        std::string variable = objJson.value("variable", "");
        size_t maxHistory = objJson.value("maxHistory", 50);
        sf::Color lineColor = jsonToColor(objJson.value("lineColor", json::array({0, 0, 255})));
        sf::Color gridColor = jsonToColor(objJson.value("gridColor", json::array({200, 200, 200, 100})));
        
        return std::make_unique<HistoryGraph>(x, y, width, height, name, db, variable, 
                                             maxHistory, lineColor, gridColor);
    }
    else if (type == "Image") {
        float width = objJson.value("width", 100.0f);
        float height = objJson.value("height", 100.0f);
        std::string path = objJson.value("path", "");
        
        return std::make_unique<Image>(x, y, width, height, path, name, db);
    }
    
    Logger::warning("Unknown object type: " + type);
    return nullptr;
}

bool JSONLoader::createDemoConfig(const std::string& filename) {
    json j;
    
    // Создаем полную демо-конфигурацию на основе вашей демо-сцены
    j["objects"] = json::array();
    
    // 1. Панель статуса (Rectangle) с условным форматированием
    json panel;
    panel["type"] = "Rectangle";
    panel["name"] = "Status Panel";
    panel["x"] = 20;
    panel["y"] = 50;
    panel["width"] = 500;
    panel["height"] = 200;
    panel["color"] = {173, 216, 230};
    panel["variable"] = "panel_status";
    panel["conditions"] = json::array({
        {{"value", 0}, {"color", {124, 36, 179}}},
        {{"value", 1}, {"color", {199, 24, 88}}},
        {{"value", 2}, {"color", {72, 146, 163}}},
        {{"value", 3}, {"color", {176, 12, 160}}},
        {{"value", 5}, {"color", {176, 80, 12}}},
        {{"value", 6}, {"color", {22, 219, 219}}},
        {{"value", 7}, {"color", {255, 0, 255}}},
        {{"value", 8}, {"color", {0, 0, 0}}},
        {{"value", 9}, {"color", {255, 0, 0}}}
    });
    j["objects"].push_back(panel);
    
    // 2. Текст температуры с динамическим обновлением
    json tempText;
    tempText["type"] = "Text";
    tempText["name"] = "Temperature Text";
    tempText["x"] = 75;
    tempText["y"] = 70;
    tempText["content"] = "Temperature: ";
    tempText["fontSize"] = 40;
    tempText["color"] = {255, 255, 255};
    tempText["variable"] = "temperature_value";
    tempText["format"] = "Temperature: ";
    j["objects"].push_back(tempText);
    
    // 3. Текст давления
    json pressureText;
    pressureText["type"] = "Text";
    pressureText["name"] = "Pressure Text";
    pressureText["x"] = 150;
    pressureText["y"] = 125;
    pressureText["content"] = "Pressure: ";
    pressureText["fontSize"] = 25;
    pressureText["color"] = {255, 255, 255};
    pressureText["variable"] = "pressure_value";
    pressureText["format"] = "Pressure: ";
    j["objects"].push_back(pressureText);
    
    // 4. Текст уставки Setpoint
    json setpointText;
    setpointText["type"] = "Text";
    setpointText["name"] = "Setpoint Text";
    setpointText["x"] = 170;
    setpointText["y"] = 160;
    setpointText["content"] = "Setpoint: ";
    setpointText["fontSize"] = 20;
    setpointText["color"] = {255, 255, 255};
    setpointText["variable"] = "setpoint_value";
    setpointText["format"] = "Setpoint: ";
    j["objects"].push_back(setpointText);
    
    // 5. Поле ввода для уставки температуры
    json inputField;
    inputField["type"] = "InputField";
    inputField["name"] = "Setpoint Input";
    inputField["x"] = 40;
    inputField["y"] = 200;
    inputField["width"] = 200;
    inputField["height"] = 40;
    inputField["fontSize"] = 23;
    inputField["variable"] = "setpoint_value";
    j["objects"].push_back(inputField);
    
    // 6. Кнопка Apply с обработчиком клика
    json applyButton;
    applyButton["type"] = "Button";
    applyButton["name"] = "Apply Button";
    applyButton["x"] = 250;
    applyButton["y"] = 200;
    applyButton["width"] = 100;
    applyButton["height"] = 40;
    applyButton["text"] = "Apply";
    applyButton["fontSize"] = 23;
    applyButton["color"] = {45, 15, 127};
    applyButton["textColor"] = {255, 255, 255};
    applyButton["action"] = "apply";
    j["objects"].push_back(applyButton);
    
    // 7. Линии-разделители
    json line1;
    line1["type"] = "Line";
    line1["name"] = "Separator Line 1";
    line1["x"] = 0;
    line1["y"] = 280;
    line1["x2"] = 1024;
    line1["y2"] = 280;
    line1["color"] = {255, 255, 255};
    j["objects"].push_back(line1);
    
    json line2;
    line2["type"] = "Line";
    line2["name"] = "Separator Line 2";
    line2["x"] = 0;
    line2["y"] = 285;
    line2["x2"] = 1024;
    line2["y2"] = 285;
    line2["color"] = {255, 255, 255};
    j["objects"].push_back(line2);
    
    json line3;
    line3["type"] = "Line";
    line3["name"] = "Separator Line 3";
    line3["x"] = 0;
    line3["y"] = 290;
    line3["x2"] = 1024;
    line3["y2"] = 290;
    line3["color"] = {255, 255, 255};
    j["objects"].push_back(line3);
    
    // 8. Прямоугольник для графика
    json graphPanel;
    graphPanel["type"] = "Rectangle";
    graphPanel["name"] = "Graph Panel";
    graphPanel["x"] = 20;
    graphPanel["y"] = 350;
    graphPanel["width"] = 400;
    graphPanel["height"] = 200;
    graphPanel["color"] = {0, 0, 0};
    graphPanel["variable"] = "";
    j["objects"].push_back(graphPanel);
    
    // 9. График истории температуры
    json graph;
    graph["type"] = "HistoryGraph";
    graph["name"] = "Temperature Graph";
    graph["x"] = 20;
    graph["y"] = 350;
    graph["width"] = 400;
    graph["height"] = 200;
    graph["variable"] = "temperature_history";
    graph["maxHistory"] = 50;
    graph["lineColor"] = {0, 255, 26};
    graph["gridColor"] = {200, 200, 200, 100};
    j["objects"].push_back(graph);
    
    // 10. Изображение
    json image;
    image["type"] = "Image";
    image["name"] = "Logo Image";
    image["x"] = 720;
    image["y"] = 330;
    image["width"] = 175;  // 350*0.5
    image["height"] = 218.5; // 437*0.5
    image["path"] = "assets/images/logo.png";
    j["objects"].push_back(image);
    
    // 11. Кнопка изменения статуса панели
    json statusButton;
    statusButton["type"] = "Button";
    statusButton["name"] = "Change Color";
    statusButton["x"] = 450;
    statusButton["y"] = 360;
    statusButton["width"] = 180;
    statusButton["height"] = 50;
    statusButton["text"] = "Change Color";
    statusButton["fontSize"] = 28;
    statusButton["color"] = {231, 214, 191};
    statusButton["textColor"] = {10, 35, 79};
    statusButton["action"] = "change_color";
    j["objects"].push_back(statusButton);
    
    // 12. Кнопка увеличения температуры 
    json tempUpButton;
    tempUpButton["type"] = "Button";
    tempUpButton["name"] = "Temp Increase";
    tempUpButton["x"] = 450;
    tempUpButton["y"] = 420;
    tempUpButton["width"] = 80;
    tempUpButton["height"] = 30;
    tempUpButton["text"] = "Temp +";
    tempUpButton["fontSize"] = 22;
    tempUpButton["color"] = {217, 72, 28};
    tempUpButton["textColor"] = {255, 255, 255};
    tempUpButton["action"] = "increase_temp";
    j["objects"].push_back(tempUpButton);
    
    // 13. Кнопка уменьшения температуры
    json tempDownButton;
    tempDownButton["type"] = "Button";
    tempDownButton["name"] = "Temp Decrease";
    tempDownButton["x"] = 540;
    tempDownButton["y"] = 420;
    tempDownButton["width"] = 80;
    tempDownButton["height"] = 30;
    tempDownButton["text"] = "Temp -";
    tempDownButton["fontSize"] = 22;
    tempDownButton["color"] = {0, 178, 232};
    tempDownButton["textColor"] = {255, 255, 255};
    tempDownButton["action"] = "decrease_temp";
    j["objects"].push_back(tempDownButton);
    
    // 14. Кнопка увеличения давления 
    json pressureUpButton;
    pressureUpButton["type"] = "Button";
    pressureUpButton["name"] = "Pressure Increase";
    pressureUpButton["x"] = 450;
    pressureUpButton["y"] = 470;
    pressureUpButton["width"] = 80;
    pressureUpButton["height"] = 30;
    pressureUpButton["text"] = "Press +";
    pressureUpButton["fontSize"] = 14;
    pressureUpButton["color"] = {143, 0, 232};
    pressureUpButton["textColor"] = {255, 255, 255};
    pressureUpButton["action"] = "increase_pressure";
    j["objects"].push_back(pressureUpButton);

    // 15. Кнопка уменьшения давления 
    json pressureDownButton;
    pressureDownButton["type"] = "Button";
    pressureDownButton["name"] = "Pressure Decrease";
    pressureDownButton["x"] = 540;
    pressureDownButton["y"] = 470;
    pressureDownButton["width"] = 80;
    pressureDownButton["height"] = 30;
    pressureDownButton["text"] = "Press -";
    pressureDownButton["fontSize"] = 14;
    pressureDownButton["color"] = {179, 73, 245};
    pressureDownButton["textColor"] = {255, 255, 255};
    pressureDownButton["action"] = "decrease_pressure";
    j["objects"].push_back(pressureDownButton);
    
    // 16. Заголовок системы
    json titleText;
    titleText["type"] = "Text";
    titleText["name"] = "System Title";
    titleText["x"] = 550;
    titleText["y"] = 20;
    titleText["content"] = "XSmall HMI Player";
    titleText["fontSize"] = 60;
    titleText["color"] = {249, 250, 197};
    j["objects"].push_back(titleText);
    
    // 17. Подзаголовок системы
    json subtitleText;
    subtitleText["type"] = "Text";
    subtitleText["name"] = "System Subtitle";
    subtitleText["x"] = 650;
    subtitleText["y"] = 90;
    subtitleText["content"] = "SCADA System";
    subtitleText["fontSize"] = 40;
    subtitleText["color"] = {255, 255, 255};
    j["objects"].push_back(subtitleText);
    
    // Записываем в файл
    std::ofstream file(filename);
    if (file.is_open()) {
        file << j.dump(4); // Красивый вывод с отступами
        Logger::info("Created demo configuration in " + filename);
        return true;
    } else {
        Logger::error("Cannot create demo configuration: " + filename);
        return false;
    }
}