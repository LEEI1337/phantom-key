#ifndef WIDGET_H
#define WIDGET_H

#include <TFT_eSPI.h>
#include "Theme.h"
#include <vector>
#include <functional>

// Basis-Klasse für alle Widgets
class Widget {
protected:
    int x, y, width, height;
    String title;
    bool visible;
    
public:
    Widget(int _x, int _y, int _w, int _h, const String& _title) 
        : x(_x), y(_y), width(_w), height(_h), title(_title), visible(true) {}
    
    virtual ~Widget() = default;
    
    virtual void draw(TFT_eSPI& tft) = 0;
    virtual void update() = 0;
    
    void setVisible(bool v) { visible = v; }
    bool isVisible() const { return visible; }
    
    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
    // Helper für abgerundete Rechtecke
    void drawCardBackground(TFT_eSPI& tft) {
        tft.fillRoundRect(x, y, width, height, Layout::RADIUS_MEDIUM, Theme::CARD_BG);
        tft.drawRoundRect(x, y, width, height, Layout::RADIUS_MEDIUM, Theme::CARD_BORDER);
    }
};

// Status Indicator Widget (LED-ähnlich)
class StatusIndicator : public Widget {
private:
    bool status;
    uint32_t colorOn;
    uint32_t colorOff;
    String label;
    
public:
    StatusIndicator(int _x, int _y, const String& _label, 
                   uint32_t _colorOn = Theme::SUCCESS, 
                   uint32_t _colorOff = Theme::ERROR)
        : Widget(_x, _y, 80, 60, ""), status(false), 
          colorOn(_colorOn), colorOff(_colorOff), label(_label) {}
    
    void setStatus(bool s) { status = s; }
    bool getStatus() const { return status; }
    
    void draw(TFT_eSPI& tft) override {
        if (!visible) return;
        
        drawCardBackground(tft);
        
        // LED Circle
        int centerX = x + width / 2;
        int centerY = y + 25;
        int radius = 15;
        
        tft.fillCircle(centerX, centerY, radius, status ? colorOn : colorOff);
        
        // Glow effect
        tft.drawCircle(centerX, centerY, radius + 2, status ? colorOn : colorOff);
        
        // Label
        tft.setTextColor(Theme::TEXT_SECONDARY, Theme::CARD_BG);
        tft.setTextSize(Fonts::SMALL);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(label, centerX, centerY + radius + 15);
    }
    
    void update() override {
        // Status wird extern gesetzt
    }
};

// Gauge Widget (Kreisförmige Anzeige)
class GaugeWidget : public Widget {
private:
    float value;      // 0.0 - 100.0
    float minValue;
    float maxValue;
    String unit;
    uint32_t color;
    
public:
    GaugeWidget(int _x, int _y, const String& _title, 
               float _min = 0, float _max = 100, 
               uint32_t _color = Theme::PRIMARY)
        : Widget(_x, _y, 120, 120, _title), 
          value(0), minValue(_min), maxValue(_max), 
          unit("%"), color(_color) {}
    
    void setValue(float v) { 
        value = constrain(v, minValue, maxValue); 
    }
    
    float getValue() const { return value; }
    
    void setUnit(const String& u) { unit = u; }
    
    void draw(TFT_eSPI& tft) override {
        if (!visible) return;
        
        drawCardBackground(tft);
        
        int centerX = x + width / 2;
        int centerY = y + height / 2;
        int radius = min(width, height) / 2 - 20;
        
        // Background circle
        tft.drawCircle(centerX, centerY, radius, Theme::CARD_BORDER);
        
        // Value arc
        float angle = map(value, minValue, maxValue, 0, 270);
        int endX = centerX + radius * cos((angle - 90) * PI / 180);
        int endY = centerY + radius * sin((angle - 90) * PI / 180);
        
        tft.drawLine(centerX, centerY, endX, endY, color);
        
        // Value text
        tft.setTextColor(Theme::TEXT_PRIMARY, Theme::CARD_BG);
        tft.setTextSize(Fonts::MEDIUM);
        tft.setTextDatum(MC_DATUM);
        tft.drawFloat(value, 1, centerX, centerY - 10);
        
        tft.setTextColor(Theme::TEXT_SECONDARY, Theme::CARD_BG);
        tft.setTextSize(Fonts::SMALL);
        tft.drawString(unit, centerX, centerY + 10);
        
        // Title
        tft.setTextColor(Theme::TEXT_PRIMARY, Theme::CARD_BG);
        tft.setTextSize(Fonts::SMALL);
        tft.setTextDatum(TC_DATUM);
        tft.drawString(title, centerX, y + 5);
    }
    
    void update() override {
        // Wert wird extern gesetzt
    }
};

// Chart Widget (Linien-Diagramm)
class ChartWidget : public Widget {
private:
    std::vector<float> data;
    size_t maxPoints;
    float minValue;
    float maxValue;
    uint32_t lineColor;
    String yAxisLabel;
    
public:
    ChartWidget(int _x, int _y, int _w, int _h, const String& _title,
               size_t _maxPoints = 60,
               float _min = 0, float _max = 100,
               uint32_t _color = Theme::ACCENT)
        : Widget(_x, _y, _w, _h, _title),
          maxPoints(_maxPoints), minValue(_min), maxValue(_max),
          lineColor(_color), yAxisLabel("") {
        data.reserve(maxPoints);
    }
    
    void addValue(float v) {
        data.push_back(constrain(v, minValue, maxValue));
        if (data.size() > maxPoints) {
            data.erase(data.begin());
        }
    }
    
    void clearData() { data.clear(); }
    
    void setYAxisLabel(const String& label) { yAxisLabel = label; }
    
    void draw(TFT_eSPI& tft) override {
        if (!visible) return;
        
        drawCardBackground(tft);
        
        // Title
        tft.setTextColor(Theme::TEXT_PRIMARY, Theme::CARD_BG);
        tft.setTextSize(Fonts::SMALL);
        tft.setTextDatum(TL_DATUM);
        tft.drawString(title, x + Layout::PADDING_MEDIUM, y + Layout::PADDING_SMALL);
        
        // Chart area
        int chartX = x + Layout::PADDING_LARGE;
        int chartY = y + 25;
        int chartW = width - Layout::PADDING_LARGE * 2;
        int chartH = height - 40;
        
        // Grid lines
        tft.setColor(Theme::CARD_BORDER);
        for (int i = 0; i <= 4; i++) {
            int gy = chartY + (chartH * i / 4);
            tft.drawFastHLine(chartX, gy, chartW);
        }
        
        // Draw line chart
        if (data.size() > 1) {
            tft.setColor(lineColor);
            int stepX = chartW / (maxPoints - 1);
            
            for (size_t i = 1; i < data.size(); i++) {
                int x1 = chartX + (i - 1) * stepX;
                int y1 = chartY + chartH - ((data[i-1] - minValue) / (maxValue - minValue) * chartH);
                int x2 = chartX + i * stepX;
                int y2 = chartY + chartH - ((data[i] - minValue) / (maxValue - minValue) * chartH);
                
                tft.drawLine(x1, y1, x2, y2);
            }
        }
        
        // Current value
        if (!data.empty()) {
            tft.setTextColor(Theme::TEXT_PRIMARY, Theme::CARD_BG);
            tft.setTextSize(Fonts::MEDIUM);
            tft.setTextDatum(TR_DATUM);
            tft.drawFloat(data.back(), 1, x + width - Layout::PADDING_MEDIUM, y + Layout::PADDING_SMALL);
        }
    }
    
    void update() override {
        // Daten werden extern hinzugefügt
    }
};

// Info Card Widget
class InfoCardWidget : public Widget {
private:
    String mainText;
    String subText;
    uint32_t iconColor;
    
public:
    InfoCardWidget(int _x, int _y, int _w, int _h, const String& _title,
                  const String& _main = "", const String& _sub = "")
        : Widget(_x, _y, _w, _h, _title),
          mainText(_main), subText(_sub), iconColor(Theme::INFO) {}
    
    void setMainText(const String& text) { mainText = text; }
    void setSubText(const String& text) { subText = text; }
    void setIconColor(uint32_t c) { iconColor = c; }
    
    void draw(TFT_eSPI& tft) override {
        if (!visible) return;
        
        drawCardBackground(tft);
        
        // Title
        tft.setTextColor(Theme::TEXT_SECONDARY, Theme::CARD_BG);
        tft.setTextSize(Fonts::SMALL);
        tft.setTextDatum(TL_DATUM);
        tft.drawString(title, x + Layout::PADDING_MEDIUM, y + Layout::PADDING_SMALL);
        
        // Main text
        tft.setTextColor(Theme::TEXT_PRIMARY, Theme::CARD_BG);
        tft.setTextSize(Fonts::LARGE);
        tft.setTextDatum(TL_DATUM);
        tft.drawString(mainText, x + Layout::PADDING_MEDIUM, y + 30);
        
        // Sub text
        if (!subText.isEmpty()) {
            tft.setTextColor(Theme::TEXT_SECONDARY, Theme::CARD_BG);
            tft.setTextSize(Fonts::SMALL);
            tft.drawString(subText, x + Layout::PADDING_MEDIUM, y + height - 20);
        }
    }
    
    void update() override {
        // Text wird extern gesetzt
    }
};

#endif // WIDGET_H
