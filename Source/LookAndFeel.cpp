#include "LookAndFeel.h"

DarkLookAndFeel::DarkLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, bgDark);
    setColour (juce::TextButton::buttonColourId, bgLight);
    setColour (juce::TextButton::textColourOffId, textBright);
    setColour (juce::TextButton::textColourOnId, accent);
    setColour (juce::Label::textColourId, textBright);
    setColour (juce::TextEditor::backgroundColourId, bgMedium);
    setColour (juce::TextEditor::textColourId, textBright);
    setColour (juce::TextEditor::outlineColourId, bgLight);
    setColour (juce::ComboBox::backgroundColourId, bgMedium);
    setColour (juce::ComboBox::textColourId, textBright);
    setColour (juce::ComboBox::outlineColourId, bgLight);
    setColour (juce::PopupMenu::backgroundColourId, bgMedium);
    setColour (juce::PopupMenu::textColourId, textBright);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
}

void DarkLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool isMouseOver, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    auto colour = backgroundColour;

    if (isButtonDown)
        colour = colour.brighter (0.2f);
    else if (isMouseOver)
        colour = colour.brighter (0.1f);

    g.setColour (colour);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (colour.brighter (0.15f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

void DarkLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                       bool /*isMouseOver*/, bool /*isButtonDown*/)
{
    auto font = juce::FontOptions (14.0f);
    g.setFont (font);
    g.setColour (button.findColour (button.getToggleState()
                                    ? juce::TextButton::textColourOnId
                                    : juce::TextButton::textColourOffId));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (4),
                      juce::Justification::centred, 1);
}
