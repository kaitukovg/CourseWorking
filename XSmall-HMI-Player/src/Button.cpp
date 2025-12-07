#include "Button.h"
#include "logger.h"

Button::Button(float x, float y, float width, float height,
               const std::string& buttonText, sf::Font* font, unsigned int fontSize,
               const sf::Color& color, const std::string& name, VariableDatabase* db,
               const std::string& varName, std::function<void()> onClickFunc,
               const sf::Color& textClr)  
    : VisualObject(x, y, name, db), font(font), variableName(varName),
      normalColor(color), 
      hoverColor(sf::Color(std::max(0, color.r - 30), std::max(0, color.g - 30), std::max(0, color.b - 30), color.a)),
      pressedColor(sf::Color(std::max(0, color.r - 50), std::max(0, color.g - 50), std::max(0, color.b - 50), color.a)),
      textColor(textClr),
      isHovered(false), isPressed(false), onClick(onClickFunc),
      actionType("") {
    
    shape.setPosition(x, y);
    shape.setSize(sf::Vector2f(width, height));
    shape.setFillColor(normalColor);
    shape.setOutlineColor(sf::Color::Black);
    shape.setOutlineThickness(1);
    
    // Центрируем текст внутри кнопки
    text.setPosition(x + width / 2, y + height / 2);
    text.setFont(*font);
    text.setString(buttonText);
    text.setCharacterSize(fontSize);
    text.setFillColor(textColor); 
    
    sf::FloatRect textBounds = text.getLocalBounds();
    text.setOrigin(textBounds.left + textBounds.width / 2, 
                   textBounds.top + textBounds.height / 2);
    
    if (!variableName.empty() && database) {
        database->subscribe(variableName, [this](double value) {
            this->update();
        });
    }
}

void Button::draw(sf::RenderWindow& window) {
    window.draw(shape);
    window.draw(text);
}

void Button::update() {
    if (!variableName.empty() && database) {
        // Кнопка, привязанная к переменной: цвет зависит от значения
        double value = database->getVariable(variableName);
        if (value == 0) {
            shape.setFillColor(normalColor);
        } else {
            shape.setFillColor(pressedColor);
        }
    } else {
        // Обычная кнопка: цвет зависит от состояние мыши
        if (isPressed) {
            shape.setFillColor(pressedColor);
        } else if (isHovered) {
            shape.setFillColor(hoverColor);
        } else {
            shape.setFillColor(normalColor);
        }
    }
}

void Button::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    bool mouseOver = contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
    
    if (event.type == sf::Event::MouseMoved) {
        isHovered = mouseOver;
        update();
    }
    
    if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left && mouseOver) {
            isPressed = true;
            update();
        }
    }
    
    if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Left && isPressed && mouseOver) {
            if (onClick) {
                onClick();
            }
            isPressed = false;
            update();
            
            // Логируем нажатие
            Logger::info("Button '" + name + "' clicked");
        } else if (isPressed) {
            isPressed = false;
            update();
        }
    }
}

bool Button::contains(float pointX, float pointY) const {
    return pointX >= x && pointX <= x + shape.getSize().x &&
           pointY >= y && pointY <= y + shape.getSize().y;
}

void Button::setOnClick(std::function<void()> onClickFunc) {
    onClick = onClickFunc;
}

void Button::setTextColor(const sf::Color& color) {
    textColor = color;
    text.setFillColor(textColor);
}

void Button::setAction(const std::string& action, VariableDatabase* db) {
    actionType = action;
    
    if (action == "change_color") {
        onClick = [db]() {
            static int status = 0;
            status = (status + 1) % 10;
            db->setVariable("panel_status", static_cast<double>(status));
            Logger::info("Panel status changed to: " + std::to_string(status));
        };
    }
    else if (action == "increase_temp") {
        onClick = [db]() {
            double current = db->getVariable("temperature_value");
            db->setVariable("temperature_value", current + 1.0);
            Logger::info("Temperature increased to: " + std::to_string(current + 1.0));
        };
    }
    else if (action == "decrease_temp") {
        onClick = [db]() {
            double current = db->getVariable("temperature_value");
            db->setVariable("temperature_value", current - 1.0);
            Logger::info("Temperature decreased to: " + std::to_string(current - 1.0));
        };
    }
    else if (action == "increase_pressure") {
        onClick = [db]() {
            double current = db->getVariable("pressure_value");
            db->setVariable("pressure_value", current + 0.5);
            Logger::info("Pressure increased to: " + std::to_string(current + 0.5));
        };
    }
    else if (action == "decrease_pressure") {
        onClick = [db]() {
            double current = db->getVariable("pressure_value");
            db->setVariable("pressure_value", current - 0.5);
            Logger::info("Pressure decreased to: " + std::to_string(current - 0.5));
        };
    }
    else if (action == "apply") {
        onClick = []() {
            Logger::info("Apply button clicked");
        };
    }
    else if (action.find("set_variable:") == 0) {
        // Новый формат: set_variable:имя_переменной=значение
        // Например: set_variable:new_rect_color=5
        size_t colon_pos = action.find(':');
        size_t equal_pos = action.find('=');
        
        if (colon_pos != std::string::npos && equal_pos != std::string::npos) {
            std::string var_name = action.substr(colon_pos + 1, equal_pos - colon_pos - 1);
            std::string value_str = action.substr(equal_pos + 1);
            
            try {
                double value = std::stod(value_str);
                onClick = [db, var_name, value]() {
                    db->setVariable(var_name, value);
                    Logger::info("Variable '" + var_name + "' set to: " + std::to_string(value));
                };
            } catch (...) {
                Logger::error("Invalid action format: " + action);
            }
        }
    }
    else if (action.find("toggle_variable:") == 0) {
        // Новый формат: toggle_variable:имя_переменной=min,max
        // Например: toggle_variable:new_rect_color=0,9
        size_t colon_pos = action.find(':');
        size_t equal_pos = action.find('=');
        size_t comma_pos = action.find(',');
        
        if (colon_pos != std::string::npos && equal_pos != std::string::npos && comma_pos != std::string::npos) {
            std::string var_name = action.substr(colon_pos + 1, equal_pos - colon_pos - 1);
            std::string min_str = action.substr(equal_pos + 1, comma_pos - equal_pos - 1);
            std::string max_str = action.substr(comma_pos + 1);
            
            try {
                int min_val = std::stoi(min_str);
                int max_val = std::stoi(max_str);
                
                onClick = [db, var_name, min_val, max_val]() {
                    double current = db->getVariable(var_name);
                    double next = static_cast<double>(((static_cast<int>(current) + 1) % (max_val + 1)));
                    if (next < min_val) next = min_val;
                    db->setVariable(var_name, next);
                    Logger::info("Variable '" + var_name + "' toggled to: " + std::to_string(next));
                };
            } catch (...) {
                Logger::error("Invalid action format: " + action);
            }
        }
    }
}
