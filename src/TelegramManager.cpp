/*
 * Copyright 2026 Komkrit Tungtatiyapat
 *
 * Personal and Educational Use Only.
 * This software is provided for educational and non-commercial purposes.
 * Any commercial use, modification for commercial purposes, manufacturing,
 * or distribution for profit is strictly prohibited without prior written
 * permission from the author.
 * * To request a commercial license, please contact: komkrit.tungtatiyapat@gmail.com
 */

#include "TelegramManager.h"

extern Preferences prefs;

void TelegramManager::begin(LogManager *sysLogger)
{
    m_sysLogger = sysLogger;
    m_client.setInsecure();
    if (bot == nullptr)
    {
        bot = new UniversalTelegramBot(SECRET_TELEGRAM_BOT_TOKEN, m_client);
    }
    if (m_sysLogger != nullptr)
    {
        m_sysLogger->sysLog("TELEGRAM", "Telegram Bot initialized");
    }
}

void TelegramManager::sendAlert(String module, String message)
{
    String text = "🚨 *Solar LED Controller ALARM*\n";
    text += "📌 *โมดูล:* " + module + "\n";
    text += "⚠️ *สถานะ:* " + message;

    bot->sendMessage(SECRET_TELEGRAM_CHAT_ID, text, "Markdown");
}

void TelegramManager::checkMessages(PowerManager *pm, TempManager *tm, FanManager *fm, LightManager *lm, OTAManager *ota)
{
    if (bot == nullptr)
        return;
    esp_task_wdt_reset();
    prefs.begin("app_info", false);
    String firmwareVersion = prefs.getString("fw_ver", "v0.0.0-dev");
    prefs.end();
    int numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    m_ota = ota;
    m_light = lm;

    while (numNewMessages)
    {
        for (int i = 0; i < numNewMessages; i++)
        {
            String chat_id = bot->messages[i].chat_id;
            String text = bot->messages[i].text;
            String type = bot->messages[i].type;

            if (type == "callback_query")
            {
                String data = text;

                if (data.startsWith("TL_") && data != "TL_OFF")
                {
                    int sH, sM, eH, eM;

                    // sscanf extracts the 4 integers from the string format "TL_18_30_06_20"
                    if (sscanf(data.c_str(), "TL_%d_%d_%d_%d", &sH, &sM, &eH, &eM) == 4)
                    {
                        // Pass the extracted times to your LightManager and enable it
                        lm->setCustomSchedule(sH, sM, eH, eM, true);

                        // Format times nicely with leading zeros (e.g., "06" instead of "6")
                        String startStr = String(sH) + ":" + (sM < 10 ? "0" : "") + String(sM);
                        String endStr = String(eH) + ":" + (eM < 10 ? "0" : "") + String(eM);

                        bot->sendMessage(chat_id, "✅ *บันทึกเวลาสำเร็จ*\nไฟจะเปิดเวลา " + startStr + " น. และปิดเวลา " + endStr + " น.", "Markdown");
                    }
                }
                else if (data == "TL_OFF")
                {
                    // --- LOAD PREFERENCES ON BOOT ---
                    prefs.begin("light_config", true);
                    sH = prefs.getInt("sHour", 18);
                    sM = prefs.getInt("sMin", 30);
                    eH = prefs.getInt("eHour", 6);
                    eM = prefs.getInt("eMin", 20);
                    isCustomScheduleActive = prefs.getBool("schActive", false);
                    prefs.end();
                    lm->setCustomSchedule(sH, sM, eH, eM, false);
                    bot->sendMessage(chat_id, "❌ *ปิดระบบตั้งเวลาแล้ว*\nไฟจะไม่เปิดอัตโนมัติตามเวลา", "Markdown");
                }
                else if (data == "FORCE_UPDATE")
                {
                    bot->sendMessage(chat_id, "⏳ กำลังเริ่มกระบวนการ OTA... ระบบจะรีบูตอัตโนมัติหากพบเวอร์ชันใหม่", "Markdown");
                    m_ota->pendingForceUpdate = true;
                }
            }
            else if (text == "/status" || text == "📊 สถานะระบบ")
            {
                float v = pm->getVoltage();
                float tLed = tm->getLedTemp();
                float tBuck = tm->getBuckTemp();
                int fanSpeed = fm->getFanSpeed();

                int batPct = 0;
                if (v >= 3.40)
                {
                    batPct = 100;
                }
                else if (v >= 3.30)
                {
                    batPct = 90 + ((v - 3.30) / 0.10) * 10; // 3.30V - 3.40V (90% - 100%)
                }
                else if (v >= 3.20)
                {
                    batPct = 30 + ((v - 3.20) / 0.10) * 60; // 3.20V - 3.30V (30% - 90%)
                }
                else if (v >= 3.10)
                {
                    batPct = 5 + ((v - 3.10) / 0.10) * 25; // 3.10V - 3.20V (5% - 30%)
                }
                else if (v >= 3.00)
                {
                    batPct = ((v - 3.00) / 0.10) * 5; // 3.00V - 3.10V (0% - 5%)
                }
                else
                {
                    batPct = 0;
                }
                batPct = constrain(batPct, 0, 100);
                String wifiSSID = WiFi.SSID();
                long rssi = WiFi.RSSI();
                String wifiStatus = (rssi > -65) ? "🟢 ดีมาก" : ((rssi > -80) ? "🟡 ปานกลาง" : "🔴 อ่อน");

                uint64_t uptimeMicros = esp_timer_get_time();
                uint64_t uptimeSecs = uptimeMicros / 1000000;
                int days = uptimeSecs / 86400;
                int hours = (uptimeSecs % 86400) / 3600;
                int mins = (uptimeSecs % 3600) / 60;
                String uptimeStr = String(days) + "d " + String(hours) + "h " + String(mins) + "m";

                String msg = "📊 *รายงานสถานะระบบ*\n\n";
                msg += "⚡ *พลังงาน:* " + String(v, 2) + "V (" + String(batPct) + "%)\n";
                msg += "💡 *ไฟ:* " + lm->isLightMode() + "\n";
                msg += "🌡️ *อุณหภูมิ:* LED " + String(tLed, 1) + "°C | Buck " + String(tBuck, 1) + "°C\n";
                msg += "🌀 *พัดลม:* " + String(fanSpeed > 0 ? "เปิด" : "ปิด") + " | ความเร็ว: " + String(fanSpeed) + "%\n";
                msg += "📶 *WiFi:* " + wifiSSID + " ความแรง: " + wifiStatus + "\n";
                msg += "⏱️ *ระยะเวลาการทำงาน:* " + uptimeStr + "\n";
                msg += "🔄 *เวอร์ชันเฟิร์มแวร์:* " + firmwareVersion + "\n";

                bot->sendMessage(chat_id, msg, "Markdown");
            }
            else if (text == "/timelight" || text == "⏱️ ตั้งเวลาแสง")
            {
                String keyboardJson = "[";
                // Option 1: 18:00 to 06:00
                keyboardJson += "[{\"text\":\"18:00 - 06:00 น.\",\"callback_data\":\"TL_18_00_06_00\"}],";
                // Option 2: 18:30 to 06:20 (Your default)
                keyboardJson += "[{\"text\":\"18:30 - 06:20 น.\",\"callback_data\":\"TL_18_30_06_20\"}],";
                // Option 3: 18:30 to 06:00
                keyboardJson += "[{\"text\":\"18:30 - 06:00 น.\",\"callback_data\":\"TL_18_30_06_00\"}],";
                // Option 4: Turn Schedule OFF
                keyboardJson += "[{\"text\":\"❌ ปิดระบบออโต้\",\"callback_data\":\"TL_OFF\"}]";
                keyboardJson += "]";

                bot->sendMessageWithInlineKeyboard(chat_id, "⚙️ *ตั้งเวลาเปิด-ปิดไฟอัตโนมัติ*\nเลือกช่วงเวลาที่คุณต้องการ:", "Markdown", keyboardJson);
            }
            else if (text == "/lighton" || text == "💡 เปิดไฟ")
            {
                lm->setManualMode(true, true); // Force manual mode ON, turn light ON
                bot->sendMessage(chat_id, "💡 *เปิดไฟและระงับโหมดออโต้ชั่วคราวแล้ว)*", "Markdown");
            }
            else if (text == "/lightoff" || text == "🌑 ปิดไฟ (กลับโหมดออโต้)")
            {
                lm->setManualMode(false, false); // Turn off manual mode, return to Schedule
                bot->sendMessage(chat_id, "🌑 *ปิดไฟและกลับสู่โหมดออโต้แล้ว*", "Markdown");
            }
            else if (text == "/dashboard" || text == "🌐 หน้าเว็บควบคุม")
            {
                String ipStr = WiFi.localIP().toString();

                String msg = "🌐 *หน้าเว็บควบคุม (ใช้ตอนเชื่อมต่อเน็ตไม่ได้)*\n\n";
                msg += "⚠️ คลิกลิงก์ด้านล่างเพื่อเข้าสู่หน้าเว็บควบคุม (คุณต้องเชื่อมต่อ WiFi  ชื่อ `T_SOLAR_LED_AP` ก่อนการใช้งาน):\n";
                msg += "👉 http://192.168.4.1\n\n";
                bot->sendMessage(chat_id, msg, "Markdown");
            }
            else if (text == "/update" || text == "🔄 ตรวจสอบอัปเดต")
            {
                bot->sendMessage(chat_id, "⏳ กำลังเชื่อมต่อกับเซิร์ฟเวอร์เพื่อตรวจสอบเวอร์ชันล่าสุด...", "Markdown");
                esp_task_wdt_reset();

                // Fetch latest release tag from GitHub API
                m_client.setTimeout(10000);
                HTTPClient http;
                http.begin(m_client, SECRET_OTA_UPDATE_API);
                http.addHeader("User-Agent", "ESP32-Telegram");

                int httpCode = http.GET();
                String latestTag = "";

                if (httpCode == HTTP_CODE_OK)
                {
                    String payload = http.getString();
                    int tagIndex = payload.indexOf("\"tag_name\":\"");
                    if (tagIndex != -1)
                    {
                        latestTag = payload.substring(tagIndex + 12);
                        latestTag = latestTag.substring(0, latestTag.indexOf("\""));
                    }
                }
                http.end();
                if (latestTag == "")
                {
                    bot->sendMessage(chat_id, "❌ *ตรวจสอบล้มเหลว*\nไม่สามารถดึงข้อมูลจากเซิร์ฟเวอร์ได้ (Error: " + String(httpCode) + ")", "Markdown");
                }
                else if (latestTag == firmwareVersion)
                {
                    String msg = "✅ *ระบบของคุณอัปเดตล่าสุดแล้ว*\n\n";
                    msg += "📌 เวอร์ชันปัจจุบัน: `" + firmwareVersion + "`\n";
                    msg += "🌐 เวอร์ชันบนคลาวด์: `" + latestTag + "`";
                    bot->sendMessage(chat_id, msg, "Markdown");
                }
                else
                {
                    String msg = "🎉 *พบเฟิร์มแวร์เวอร์ชันใหม่!*\n\n";
                    msg += "📌 เวอร์ชันปัจจุบัน: `" + firmwareVersion + "`\n";
                    msg += "🆕 เวอร์ชันใหม่: `" + latestTag + "`\n\n";
                    msg += "ต้องการอัปเดตระบบตอนนี้เลยไหม?";
                    String keyboardJson = "[[{\"text\":\"🚀 ยืนยันอัปเดตเป็น " + latestTag + "\",\"callback_data\":\"FORCE_UPDATE\"}]]";
                    bot->sendMessageWithInlineKeyboard(chat_id, msg, "Markdown", keyboardJson);
                }
            }
            else if (text == "/rollback" || text == "↩️ ย้อนเวอร์ชันอัปเดต")
            {
                m_ota->triggerRollback();
            }
            else
            {
                String keyboardJson = "[";
                keyboardJson += "[\"💡 เปิดไฟ\", \"🌑 ปิดไฟ (กลับโหมดออโต้)\"],";
                keyboardJson += "[\"⏱️ ตั้งเวลาแสง\", \"📊 สถานะระบบ\"],";
                keyboardJson += "[\"🔄 ตรวจสอบอัปเดต\",\"↩️ ย้อนเวอร์ชันอัปเดต\"],";
                keyboardJson += "[\"🌐 หน้าเว็บควบคุม\"]";
                keyboardJson += "]";
                bot->sendMessageWithReplyKeyboard(chat_id, "👇 เลือกคำสั่งจากปุ่มด้านล่างได้เลย", "", keyboardJson, true);
            }
        }
        esp_task_wdt_reset();
        numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    }
}