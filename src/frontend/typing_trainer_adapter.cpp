#include "typing_trainer_adapter.hpp"
#include <QMetaObject>

namespace typing_trainer
{

QmlTypingTrainerAdapter::QmlTypingTrainerAdapter(QObject* parent)
    : QObject(parent)
    , m_core(std::make_unique<TypingTrainerCore>())
{
    // Настраиваем колбэк от ядра
    m_core->set_output_ready_callback([this]() {
        // ВАЖНО: Колбэк вызывается из фонового потока std::jthread.
        // Мы используем QMetaObject::invokeMethod с параметром Qt::QueuedConnection,
        // чтобы безопасно перенаправить выполнение метода onOutputReady в UI-поток Qt.
        QMetaObject::invokeMethod(this, "onOutputReady", Qt::QueuedConnection);
    });
}

void QmlTypingTrainerAdapter::startSession(const QString& text)
{
    StartSessionCommand cmd;
    cmd.config.mode = TrainingMode::Free;
    cmd.config.custom_text = text.toStdU32String();
    m_core->push_input(cmd);
}

void QmlTypingTrainerAdapter::sendKeyPress(const QString& keyText)
{
    if (keyText.isEmpty()) return;
    
    KeyPressData data;
    data.key = static_cast<char32_t>(keyText.at(0).unicode());
    data.timestamp = std::chrono::steady_clock::now();
    m_core->push_input(data);
}

void QmlTypingTrainerAdapter::sendBackspace()
{
    KeyPressData data;
    data.key = ControlKey::Backspace;
    data.timestamp = std::chrono::steady_clock::now();
    m_core->push_input(data);
}

void QmlTypingTrainerAdapter::onOutputReady()
{
    // Этот метод гарантированно выполняется в главном UI-потоке Qt
    while (auto event_opt = m_core->poll_output())
    {
        std::visit([this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            
            if constexpr (std::is_same_v<T, SessionState>)
            {
                // 1. Исправление: Собираем std::u32string из вектора CharState
                std::u32string u32_text;
                u32_text.reserve(arg.chars.size());
                for (const auto& char_state : arg.chars)
                {
                    u32_text.push_back(char_state.character);
                }

                // Преобразуем UTF-32 строку в QString для QML
                m_textToType = QString::fromStdU32String(u32_text);
                
                m_cursorPosition = static_cast<int>(arg.cursor_position);
                m_wpm = arg.metrics.wpm;
                m_accuracy = arg.metrics.accuracy;
                
                emit textToTypeChanged();
                emit cursorPositionChanged();
                emit metricsChanged();
            }
            else if constexpr (std::is_same_v<T, StateUpdate>)
            {
                m_cursorPosition = static_cast<int>(arg.cursor_position);
                m_wpm = arg.metrics.wpm;
                m_accuracy = arg.metrics.accuracy;
                
                emit cursorPositionChanged();
                emit metricsChanged();
                
                if (arg.is_completed) {
                    emit sessionCompleted();
                }
            }
        }, *event_opt);
    }
}

} // namespace typing_trainer