#ifndef JSONLOADER_H
#define JSONLOADER_H

#include <string>
#include <vector>
#include <memory>
#include "VariableDatabase.h"
#include "VisualObject.h"

// Простое объявление - заголовок будет скачан отдельно
#include <nlohmann/json.hpp>

namespace sf {
    class Font;
}

class JSONLoader {
public:
    // Загружает объекты из JSON файла
    static std::vector<std::unique_ptr<VisualObject>> loadFromFile(
        const std::string& filename, 
        VariableDatabase* db,
        sf::Font* font);
    
    // Создает демо-сцену и сохраняет в JSON
    static bool createDemoConfig(const std::string& filename);
    
private:
    // Создает объект из JSON
    static std::unique_ptr<VisualObject> createObject(
        const nlohmann::json& objJson,
        VariableDatabase* db,
        sf::Font* font);
    
    // Вспомогательные функции
    static sf::Color jsonToColor(const nlohmann::json& colorJson);
    static nlohmann::json colorToJson(const sf::Color& color);
};

#endif