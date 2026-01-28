#define VERSION 1.0
//22.01.2025

// Библиотеки
#include "buttonMinim.h"
#include "src/microWire/microWire.h"
#include "src/SSD1306Ascii/src/SSD1306AsciiWire.h"

// Пины
#define BTN_UP 3
#define BTN_SET 4
#define BTN_DWN 2
#define OLED_PWR1 5
#define OLED_PWR2 6
#define MENU_LANG 0 //0 - Русский, 1 - Английский Переключать толку нет, не реализовано
#define BIG_NUM_FONT FixedNum30x40 //Шрифт для цифр счётчика
#define MAIN_FONT_RU Rus5x8
#define MAIN_FONT_EN X11fixed7x14
#define DISP_WIDTH 128  // ширина дисплея в пикселях

// Данные и классы
// === МОЖНО МЕНЯТЬ ===
uint16_t counter = 50; // Счётчик здоровья героя (начальная позиция)
uint16_t altCounter = 25;  // Отдельный счётчик для альт. формы
uint16_t bonusCounter = 0;  // Отдельный счётчик для временного hp
// === НЕЛЬЗЯ МЕНЯТЬ ===
buttonMinim buttUP(BTN_UP); //Инициализация кноки Вверх
buttonMinim buttSET(BTN_SET); //Инициализация кноки SET
buttonMinim buttDWN(BTN_DWN); //Инициализация кноки Вниз
SSD1306AsciiWire disp; //Инициализация дисплея
static uint16_t lastCounter = 50; //Последнее состояние счётчика здоровья героя
static uint8_t lastDigits = 2;  //Последнее число цифр на дисплее, для зачистки
unsigned long lastUP = 0, lastDWN = 0; //Тайминги удержания кнопок для подсчёта
bool upHeld = false, dwnHeld = false; //Переключатели удержания для подсчёта
unsigned long upHoldStart = 0, dwnHoldStart = 0;  // ВРЕМЯ НАЧАЛА УДЕРЖАНИЯ
// === ДЛЯ РЕЖИМА ОЖИДАНИЯ ===
unsigned long lastActivity = 0;  // Последнее нажатие кнопки
const unsigned long IDLE_TIMEOUT = 5UL * 10 * 1000;  // Время до ухода в режим ожидания, хз как считается, тут примерно 5 минут
bool idleMode = false; //Режим ожидания
bool clearBonus = false; //Для затирки временного HP
uint8_t idleX, idleY, waitIcon = 1;  // idleX, idleY Случайная позиция для &, waitIcon тип иконки
unsigned long lastIdleMove = 0; // Тайминг последнего движения иконки ожидания
unsigned long now;
// === МЕНЮ ===
enum AppMode {
  MODE_HP_HERO = 0,      // Здоровье героя
  MODE_ALT_FORM = 1,      // Альтернативная форма
  MODE_BONUS_HP = 2       // Временное здоровье
};
AppMode currentMode = MODE_HP_HERO;
uint8_t menuSelected = 0;  // 0 или 1 или 2 в меню
bool inMenu = false;

// выравнивание текста
enum text_position { 
  Append = -4,  
  Left,         
  Center,       
  Right         
};

// расчёт ширины текста
uint8_t strWidth(const char str[]) {
  uint8_t _width = 0;
  while (*str) {
    _width += disp.charWidth(*str++);
  }
  return _width;
}

// вывод текста с выравниванием
void printStr(const char str[], int8_t x = Append, int8_t y = Append) {
  switch (x) {
    case Left:   disp.setCol(0); break;
    case Center: disp.setCol((DISP_WIDTH - strWidth(str)) / 2); break;
    case Right:  disp.setCol(DISP_WIDTH - strWidth(str)); break;
    default:     disp.setCol(x); break;
  }
  if (y != Append) disp.setRow(y);
  while (*str) disp.write((uint8_t)*str++);
  //while (*str) disp.write(*str++);
}

// вывод числа с выравниванием
void printInt(uint16_t num, int8_t x = Append, int8_t y = Append) {
  char cstr[6];
  itoa(num, cstr, 10);
  printStr(cstr, x, y);
}

void setup() {
  // Включаем питание OLED
  pinMode(OLED_PWR1, OUTPUT);
  pinMode(OLED_PWR2, OUTPUT);
  digitalWrite(OLED_PWR1, HIGH);
  digitalWrite(OLED_PWR2, HIGH);
  delay(100);
  oledInit();
  delay(500);
  disp.clear();
  testAllLetters();
  delay(500);
  disp.clear();
  disp.setFont(MAIN_FONT_RU);
  printStr("  Здоровье героя", Left, 7);
  disp.setFont(BIG_NUM_FONT);
  printInt(counter, Center, 1);
  lastActivity = millis();  // ИНИЦИАЛИЗАЦИЯ таймера
}

// ================ ИНИЦИАЛИЗАЦИЯ ================
void oledInit() {
  Wire.begin();
  Wire.setClock(400000L);
  disp.begin(&Adafruit128x64, 0x3C);
  #if(MENU_LANG == 0)
  disp.setFont(MAIN_FONT_RU);
  #elif (MENU_LANG == 1)
  disp.setFont(MAIN_FONT_EN);
  #endif
  disp.setContrast(3);
  disp.clear();
  disp.home();
  drawDNDLogo();
  disp.setFont(MAIN_FONT_RU);
  printStr("v1.0   ", Right, 7);
}


// Тест всех букв с анимацией при загрузке
void testAllLetters() {
    disp.setFont(MAIN_FONT_RU);
    // Имитация загрузки
    disp.clear();
    printStr("Инициализация...", Left, 1);
    printStr("================", Left, 2);
    delay(10);
    
    printStr("Заглавные русские:", Left, 3);
    printStr("АБВГДЕЖЗИЙКЛМНОП..", Left, 4);
    printStr("РСТУФХЦЧШЩЪЫЬЭЮЯ..", Left, 5);
    delay(80);

    printStr("....................", Left, 3);
    printStr("....................", Left, 4);
    printStr("....................", Left, 5);
    delay(20);

    printStr("Строчные русские: ", Left, 3);
    printStr("абвгдежзийклмноп..", Left, 4);
    printStr("рстуфхцчшщъыьэюя..", Left, 5);
    delay(80);

    printStr("....................", Left, 3);
    printStr("....................", Left, 4);
    printStr("....................", Left, 5);
    delay(20);

    printStr("Англ. заглавные:  ", Left, 3);
    printStr("ABCDEFGHIJKLMNO...", Left, 4);
    printStr("PQRSTUVWXYZ.......", Left, 5);
    delay(100);

    printStr("....................", Left, 3);
    printStr("....................", Left, 4);
    printStr("....................", Left, 5);
    delay(20);

    printStr("Англ. строчные:   ", Left, 3);
    printStr("abcdefghijklmno...", Left, 4);
    printStr("pqrstuvwxyz.......", Left, 5);
    delay(200);
    printStr("....................", Left, 3);
    printStr("....................", Left, 4);
    printStr("....................", Left, 5);
    
    printStr("Все проверки", Left, 6);
    printStr("пройдены успешно!!!", Left, 7);
    delay(200);
}

// Функция режима ожидания
void enterIdleMode() {
  idleMode = true;
  lastIdleMove = millis();
  drawIdleSymbolRandom();
}

//Рисует символ ожидания в рандомном месте
void drawIdleSymbolRandom() {
  disp.clear();
  disp.setFont(DNDand);

  // Случайная позиция для символа &
  idleX = random(1, 110);  // 1-110 пикселей по X
  idleY = random(1, 55);    // 1-55 пикселей по Y

  if (idleX > 77) idleX = 77; //Чтобы амперсант не вылезал за границы экарана
  if (idleY > 14) idleY = 14; //Чтобы амперсант не вылезал за границы экарана
  
  disp.setCursor(idleX, idleY);
  printInt(waitIcon);
}

//Выход из режима ожидания
void exitIdleMode() {
  idleMode = false;
  disp.clear();
  switch (currentMode){
    case MODE_HP_HERO:
      if (bonusCounter == 0) {
        disp.setFont(MAIN_FONT_RU);
        printStr("  Здоровье героя", Left, 7);
        disp.setFont(BIG_NUM_FONT);
        printInt(counter, Center, 1);
      } else {
        disp.setFont(BIG_NUM_FONT);
        printInt(counter, Center, 1);
        disp.setFont(MAIN_FONT_RU);
        printStr("Временное HP:", Left, 7);
        printInt(bonusCounter, 100, 7);
      }      
      break;
    case MODE_ALT_FORM:
      disp.setFont(MAIN_FONT_RU);
      printStr("    Альт. форма", Left, 7);
      disp.setFont(BIG_NUM_FONT);
      printInt(counter, Center, 1);
      break;
    case MODE_BONUS_HP:
      disp.setFont(MAIN_FONT_RU);
      printStr("Временное HP:", Left, 7);
      disp.setFont(BIG_NUM_FONT);
      printInt(counter, Center, 1);
      break;
    default:
      disp.setFont(MAIN_FONT_RU);
      printStr("  Здоровье героя", Left, 7);
      disp.setFont(BIG_NUM_FONT);
      printInt(counter, Center, 1);
      break;
  }
}

//Включает логотип DND
void drawDNDLogo() {
  disp.setFont(DNDlogo);
  printInt(0, Center, 1);
}

//Включает значок гроба один из 4
void drawDNDdie() {
  disp.setFont(DNDand); //У шрифта 6 символов, символ 0 пустой, для затирки
  waitIcon = random(2, 5);
  printInt(waitIcon, Center, 1);
}

//Обновляем дисплей
void updateDisplay() {
  if (idleMode || inMenu) return;
  if (currentMode == MODE_HP_HERO && counter == lastCounter) return;
  if (currentMode == MODE_ALT_FORM && altCounter == lastCounter) return;
  if (currentMode == MODE_BONUS_HP && bonusCounter == lastCounter) return;
  uint16_t* currentValuePtr;
  // Обновляем lastCounter для текущего режима
  //uint16_t* currentValuePtr = (currentMode == MODE_HP_HERO) ? &counter : &altCounter; //Указатель на переменную
  switch (currentMode){
    case MODE_HP_HERO:
      currentValuePtr = (bonusCounter > 0) ? &bonusCounter : &counter; break;
    case MODE_ALT_FORM:
      currentValuePtr = &altCounter; break;
    case MODE_BONUS_HP:
      currentValuePtr = &bonusCounter; break;
    default:
      currentValuePtr = &counter; break;
  }
  
  // Считаем текущие цифры
  uint8_t currDigits = 1;
  uint16_t temp = *currentValuePtr;
  if (temp >= 100) currDigits = 3;
  else if (temp >= 10) currDigits = 2;

  // Очищаем ТОЛЬКО при уменьшении разрядности
  if (currDigits < lastDigits) {
    if(currentMode == MODE_HP_HERO && bonusCounter > 0) {
      printStr("   ", 100, 7);
    } else {
      if (currDigits == 2 || (currDigits == 1 && lastDigits == 3)) printStr("   ", Center, 1);
      else if (currDigits == 1) printStr("  ", Center, 1);
    } 
  }

  //Иконка смерти при достижении нуля
  if (currentMode == MODE_HP_HERO || currentMode == MODE_ALT_FORM) {
    if(currentMode == MODE_HP_HERO && bonusCounter > 0) {
      printInt(*currentValuePtr, 100, 7);
    } else {
      if (clearBonus == true) {
        clearBonus = false;
        printStr("  Здоровье героя   ", Left, 7);
        disp.setFont(BIG_NUM_FONT);
      }
      if (*currentValuePtr == 0) {
        drawDNDdie();
      } else {
        if (*currentValuePtr == 1 && lastCounter == 0) {
          printInt(0, Center, 1); //Стираем гробы
          waitIcon = 1; //Сбрасываем знак ожидания
          disp.setFont(BIG_NUM_FONT); //И возвращаем шрифт
        }
        printInt(*currentValuePtr, Center, 1);
      }
    }
  } else printInt(*currentValuePtr, Center, 1);

  lastDigits = currDigits;
  lastCounter = *currentValuePtr;
}

//Возврат к счётчикам
void showMainScreen() {
  disp.clear();
  if (currentMode == MODE_HP_HERO) {
    if (bonusCounter == 0){
      disp.setFont(MAIN_FONT_RU);
      printStr("  Здоровье героя", Left, 7);
      disp.setFont(BIG_NUM_FONT);
      printInt(counter, Center, 1);
    } else {
      disp.setFont(BIG_NUM_FONT);
      printInt(counter, Center, 1);
      disp.setFont(MAIN_FONT_RU);
      printStr("Временное HP:", Left, 7);
      printInt(bonusCounter, 100, 7);
    }
  } else if (currentMode == MODE_ALT_FORM) {
    disp.setFont(MAIN_FONT_RU);
    printStr("    Альт. форма", Left, 7);
    disp.setFont(BIG_NUM_FONT);
    printInt(altCounter, Center, 1);
  } else if (currentMode == MODE_BONUS_HP) {
    printStr("Времменное HP:", Left, 1);
    printStr("Добавьте нужное", Left, 3);
    printStr("кол-во здоровья", Left, 4);
    printStr("и вернитесь назад.", Left, 5);
    delay(300);
    disp.clear();
    disp.setFont(MAIN_FONT_RU);
    printStr("Времменное HP", Left, 7);
    disp.setFont(BIG_NUM_FONT);
    printInt(bonusCounter, Center, 1);
  }
}

//Выход в меню
void showMenu() {
  disp.clear();
  disp.setFont(MAIN_FONT_RU);
  printStr("Меню", Left, 0);
  printStr("----------------------", Left, 1);
  printStr(" Здоровье героя", Left, 2);
  printStr(" Альт. форма", Left, 3);
  printStr(" Врем. здоровье", Left, 4);
  printStr("----------------------", Left, 6);
  printStr("выберите пункт", Left, 7);
  menuSelect();
}

//Селектор меню
void menuSelect() {
  if (menuSelected == 0) {
    printStr(">", Left, 2);
    printStr(" ", Left, 3);
    printStr(" ", Left, 4);
  }
  if (menuSelected == 1) {
    printStr(" ", Left, 2);
    printStr(">", Left, 3);
    printStr(" ", Left, 4);
  }
  if (menuSelected == 2) {
    printStr(" ", Left, 2);
    printStr(" ", Left, 3);
    printStr(">", Left, 4);
  }
}



void loop() {
  now = millis();
  
  // TIMEOUT проверка
  if (!idleMode && !inMenu && (now - lastActivity >= IDLE_TIMEOUT)) {
    enterIdleMode();
  }
  
  // ВЫХОД ИЗ IDLE
  if (idleMode && (buttUP.pressed() || buttDWN.pressed() || buttSET.pressed())) {
    exitIdleMode();
    lastActivity = now;
    updateDisplay();
    //showMainScreen();  // Показываем текущий экран
    return;
  }
  
  // АНИМАЦИЯ IDLE
  if (idleMode) {
    if (now - lastIdleMove >= 3000) {
      drawIdleSymbolRandom();
      lastIdleMove = now;
    }
    return;
  }
  
  // tick() для всех кнопок
  buttUP.tick();
  buttDWN.tick();
  buttSET.tick();

  // Проверка активности для режима ожидания
  static unsigned long lastCheck = 0;
  if (now - lastCheck >= 1) {
    bool anyPressed = (buttUP.pressed() || buttDWN.pressed() || buttSET.pressed());
    if (anyPressed) lastActivity = now;
    lastCheck = now;
  }
  
  // ==================== МЕНЮ ====================
  if (inMenu) {
    if (buttUP.pressed()) {
      if (menuSelected > 0) menuSelected--;
      menuSelect();
    }
    if (buttDWN.pressed()) {
      if (menuSelected < 2) menuSelected++;
      menuSelect();
    }
    if (buttSET.pressed()) {
      currentMode = (AppMode)menuSelected;
      inMenu = false;
      showMainScreen();
    }
    return;
  }
  
  // ==================== ГЛАВНЫЙ ЭКРАН ====================
  // КНОПКА SET → МЕНЮ
  if (buttSET.pressed()) {
    if (bonusCounter > 0) clearBonus = true;
    inMenu = true;
    showMenu();
    return;
  }
  
  // UP/DOWN для текущего счётчика
  uint16_t* currentValuePtr; // = (currentMode == MODE_HP_HERO) ? &counter : &altCounter;
  switch (currentMode){
    case MODE_HP_HERO:
      currentValuePtr = (bonusCounter > 0) ? &bonusCounter : &counter; break;
    case MODE_ALT_FORM:
      currentValuePtr = &altCounter; break;
    case MODE_BONUS_HP:
      currentValuePtr = &bonusCounter; break;
    default:
      currentValuePtr = &counter; break;
  }
  
  // ========== UP ==========
  if (buttUP.pressed()) {
    *currentValuePtr = min(*currentValuePtr + 1, 999);
    upHoldStart = now;
    updateDisplay();
    lastUP = 0;
  }
  
  if (buttUP.holding()) {
    uint32_t totalHoldTime = now - upHoldStart;  // ОБЩЕЕ ВРЕМЯ УДЕРЖАНИЯ
    if (totalHoldTime > 300 && now - lastUP > 50) { 
      *currentValuePtr = min(*currentValuePtr + 100, 999);
      lastUP = now;
      updateDisplay();
    } 
    else if (totalHoldTime > 80 && now - lastUP > 50) {
      *currentValuePtr = min(*currentValuePtr + 10, 999);
      lastUP = now;
      updateDisplay();
    }
  }
  
  // ========== DWN ==========
  if (buttDWN.pressed()) {
    if (*currentValuePtr > 0) {
      *currentValuePtr = max(*currentValuePtr - 1, 0);
      updateDisplay();
    }
    dwnHoldStart = now;
    lastDWN = 0;
  }

  if (buttDWN.holding()) {
    uint32_t totalHoldTime = now - dwnHoldStart;
    
    if (totalHoldTime > 300 && now - lastDWN > 50) {
      // УМНАЯ ЛОГИКА: проверяем остаток
      if (*currentValuePtr == 0) {
      // Блокируем изменения когда на минимуме
      } else if (*currentValuePtr >= 100) {
        *currentValuePtr = max(*currentValuePtr - 100, 0);  // -100 если >=100
      } else if (*currentValuePtr >= 10) {
        *currentValuePtr = max(*currentValuePtr - 10, 0);   // -10 если 10-99
      } else {
        *currentValuePtr = max(*currentValuePtr - 1, 0);    // -1 если <10
      }
      lastDWN = now;
      updateDisplay();
    } 
    else if (totalHoldTime > 80 && now - lastDWN > 50) {
      // УМНАЯ ЛОГИКА для среднего режима
      if (*currentValuePtr == 0) {
      // Блокируем изменения когда на минимуме
      } else if (*currentValuePtr >= 10) {
        *currentValuePtr = max(*currentValuePtr - 10, 0);   // -10 если >=10
      } else {
        *currentValuePtr = max(*currentValuePtr - 1, 0);    // -1 если <10
      }
      lastDWN = now;
      updateDisplay();
    }
  }
}
