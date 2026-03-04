#include "TelegramManager.h"

void TelegramManager::begin(LogManager* sysLogger) {
    m_sysLogger = sysLogger;
    client.setInsecure();
    bot = new UniversalTelegramBot(TELEGRAM_BOT_TOKEN, client);
    if (m_sysLogger != nullptr) {
        m_sysLogger->sysLog("TELEGRAM", "Telegram Bot initialized");
    }
}

void TelegramManager::sendAlert(String module, String message) {
    String text = "🚨 *Solar LED Controller ALARM*\n";
    text += "📌 *Module:* " + module + "\n";
    text += "⚠️ *Status:* " + message;
    
    bot->sendMessage(TELEGRAM_CHAT_ID, text, "Markdown");
}