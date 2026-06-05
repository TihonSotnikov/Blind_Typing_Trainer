import QtQuick
import QtQuick.Controls
import BlindTypingTrainerModule  1.0

ApplicationWindow {
    visible: true
    width: 600
    height: 400
    title: "Typing Trainer"

    // Инициализируем наше ядро через адаптер
    TypingTrainerCore {
        id: trainer
        onSessionCompleted: {
            statusLabel.text = "Session Completed!"
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 20

        // Текст для ввода
        Text {
            text: trainer.textToType
            font.pointSize: 18
            // Здесь можно добавить логику подсветки текущего символа 
            // на основе trainer.cursorPosition
        }

        // Вывод метрик (обновляются автоматически при изменении свойств в C++)
        Row {
            spacing: 20
            Text { text: "WPM: " + trainer.wpm.toFixed(1) }
            Text { text: "Accuracy: " + trainer.accuracy.toFixed(1) + "%" }
            Text { text: "Cursor: " + trainer.cursorPosition }
        }

        Label {
            id: statusLabel
            text: "Press 'Start' to begin"
        }

        Button {
            text: "Start"
            onClicked: {
                statusLabel.text = "Typing..."
                trainer.startSession("example text for training")
                inputField.forceActiveFocus()
            }
        }

        // Невидимое поле для перехвата ввода клавиатуры
        Item {
            id: inputField
            focus: true
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Backspace) {
                    trainer.sendBackspace();
                    event.accepted = true;
                } else if (event.text.length > 0) {
                    trainer.sendKeyPress(event.text);
                    event.accepted = true;
                }
            }
        }
    }
}