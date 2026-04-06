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
    String firmwareVersion = prefs.getString("fw_ver", "v0.2.1");
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
            String query_id = bot->messages[i].query_id;

            if (type == "callback_query")
            {
                String data = text;
                if (chat_id == "")
                    chat_id = SECRET_TELEGRAM_CHAT_ID;
                bot->answerCallbackQuery(query_id);
                esp_task_wdt_reset();
                int sH = 0, sM = 0, eH = 0, eM = 0;
                if (data == "RB_CONFIRM")
                {
                    bot->sendMessage(chat_id, "⚙️ *System:* กำลังเริ่มกระบวนการ Rollback กรุณารอสักครู่...", "Markdown");
                    m_ota->triggerRollback();
                }
                else if (data == "TL_OFF_REQ")
                {
                    String confirmKb = "[[{\"text\":\"✅ ใช่, ยืนยันปิดออโต้\",\"callback_data\":\"TL_OFF_CONFIRM\"}]]";
                    bot->sendMessageWithInlineKeyboard(chat_id, "⚠️ *ยืนยันการปิดระบบ*\nคุณแน่ใจว่าต้องการปิดระบบตั้งเวลาแสงอัตโนมัติ?", "Markdown",confirmKb);
                }
                else if (data == "TL_OFF_CONFIRM") 
                {
                    prefs.begin("light_config", false);
                    prefs.putBool("schActive", false);
                    prefs.end();
                    
                    lm->setCustomSchedule(18, 30, 6, 20, false); 
                    bot->sendMessage(chat_id, "❌ *ปิดระบบตั้งเวลาเรียบร้อยแล้ว*\nไฟจะไม่เปิดอัตโนมัติตามเวลา", "Markdown");
                }
                else if (data == "FORCE_UPDATE")
                {
                    bot->sendMessage(chat_id, "⏳ *กำลังเริ่มกระบวนการ OTA...* ระบบจะรีบูตอัตโนมัติ", "Markdown");
                    m_ota->pendingForceUpdate = true;
                }
                else if (data.startsWith("TL_") && data != "TL_OFF")
                {
                    // sscanf extracts the 4 integers from the string format "TL_18_30_06_20"
                    if (sscanf(data.c_str(), "TL_%d_%d_%d_%d", &sH, &sM, &eH, &eM) == 4)
                    {
                        // Save to NVS so Web Dashboard and Reboots stay perfectly synced
                        prefs.begin("light_config", false);
                        prefs.putInt("sHour", sH);
                        prefs.putInt("sMin", sM);
                        prefs.putInt("eHour", eH);
                        prefs.putInt("eMin", eM);
                        prefs.putBool("schActive", true);
                        prefs.end();
                        // Pass the extracted times to your LightManager and enable it
                        lm->setCustomSchedule(sH, sM, eH, eM, true);

                        // Format times nicely with leading zeros (e.g., "06" instead of "6")
                        String startStr = String(sH) + ":" + (sM < 10 ? "0" : "") + String(sM);
                        String endStr = String(eH) + ":" + (eM < 10 ? "0" : "") + String(eM);

                        bot->sendMessage(chat_id, "✅ *บันทึกเวลาสำเร็จ*\nไฟจะเปิดเวลา " + startStr + " น. และปิดเวลา " + endStr + " น.", "Markdown");
                    }
                }
            }
            else if (text == "/status" || text == "📊 สถานะระบบ")
            {
                float v = pm->getVoltage();
                float tBuck = tm->getBuckTemp();
                int fanSpeed = fm->getFanSpeed();

                int batPct = 0;

                // LiFePO4 1S (3.2V Nominal) Discharge Curve Approximation
                if (v >= 3.45)
                {
                    batPct = 100; // Fully charged or actively receiving strong solar charge
                }
                else if (v >= 3.35)
                {
                    batPct = 90 + ((v - 3.35) / 0.10) * 10; // 3.35V - 3.45V (90% - 100%)
                }
                else if (v >= 3.25)
                {
                    batPct = 70 + ((v - 3.25) / 0.10) * 20; // 3.25V - 3.35V (70% - 90%)
                }
                else if (v >= 3.20)
                {
                    // The "Flat Zone" - LiFePO4 spends most of its life right here
                    batPct = 30 + ((v - 3.20) / 0.05) * 40; // 3.20V - 3.25V (30% - 70%)
                }
                else if (v >= 3.10)
                {
                    batPct = 10 + ((v - 3.10) / 0.10) * 20; // 3.10V - 3.20V (10% - 30%)
                }
                else if (v >= 2.80)
                {
                    // Sharp drop-off zone. 2.8V is a safe practical cut-off for longevity.
                    batPct = ((v - 2.80) / 0.30) * 10; // 2.80V - 3.10V (0% - 10%)
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
                String uptimeStr = String(days) + "วัน " + String(hours) + "ชม " + String(mins) + "นาที";

                String msg = "📊 *รายงานสถานะระบบ*\n\n";
                msg += "⚡ *พลังงาน:* " + String(v, 2) + "V (" + String(batPct) + "%)\n";
                msg += "💡 *ไฟ:* " + lm->isLightMode() + "\n";
                msg += "🌡️ *อุณหภูมิ:* วงจรลดแรงดัน " + String(tBuck, 1) + "°C\n";
                msg += "🌀 *พัดลม:* " + String(fanSpeed > 0 ? "เปิด" : "ปิด") + " | ความเร็ว: " + String(fanSpeed) + "%\n";
                msg += "📶 *WiFi:* " + wifiSSID + " ความแรง: " + wifiStatus + "\n";
                msg += "⏱️ *ระยะเวลาการทำงาน:* " + uptimeStr + "\n";
                msg += "🔄 *เวอร์ชันเฟิร์มแวร์:* " + firmwareVersion + "\n";

                bot->sendMessage(chat_id, msg, "Markdown");
            }
            else if (text == "/log" || text == "📝 ดู Log ล่าสุด")
            {
                if (m_sysLogger != nullptr)
                {
                    bot->sendMessage(chat_id, "⏳ กำลังดึงข้อมูล Log...", "");
                    esp_task_wdt_reset();

                    String logData = m_sysLogger->getTailLogs(800);
                    String msg = "<b>📜 System Logs (ล่าสุด):</b>\n<pre>" + logData + "</pre>";

                    bot->sendMessage(chat_id, msg, "HTML");
                }
                else
                {
                    bot->sendMessage(chat_id, "❌ ระบบ Log Manager ไม่พร้อมทำงาน", "");
                }
            }
            else if (text == "/timelight" || text == "⏱️ ตั้งเวลาแสง")
            {
                prefs.begin("light_config", true); // true = Read Only
                int cur_sH = prefs.getInt("sHour", 18);
                int cur_sM = prefs.getInt("sMin", 30);
                int cur_eH = prefs.getInt("eHour", 6);
                int cur_eM = prefs.getInt("eMin", 20);
                bool isActive = prefs.getBool("schActive", false);
                prefs.end();

                String sTime = String(cur_sH) + ":" + (cur_sM < 10 ? "0" : "") + String(cur_sM);
                String eTime = String(cur_eH) + ":" + (cur_eM < 10 ? "0" : "") + String(cur_eM);
                String statusTxt = isActive ? "🟢 *กำลังเปิดใช้งาน*" : "🔴 *ปิดใช้งาน (Manual)*";
                
                String msg = "⚙️ *แผงควบคุมเวลาแสง (Timer)*\n";
                msg += "━━━━━━━━━━━━━━━━\n";
                msg += "📌 *สถานะปัจจุบัน:* " + statusTxt + "\n";
                if (isActive) {
                    msg += "⏰ *เวลาที่ตั้งไว้:* `" + sTime + " น.` ถึง `" + eTime + " น.`\n";
                }
                msg += "━━━━━━━━━━━━━━━━\n";
                msg += "👇 *เลือกปรับช่วงเวลาใหม่ที่ต้องการ:*";

                String kb = "[";
                kb += "[{\"text\":\"🌙 18:00 - 06:00\",\"callback_data\":\"TL_18_00_06_00\"},";
                kb += " {\"text\":\"🌙 18:30 - 06:00\",\"callback_data\":\"TL_18_30_06_00\"}],";
                kb += "[{\"text\":\"🌃 18:30 - 06:20\",\"callback_data\":\"TL_18_30_06_20\"},";
                kb += " {\"text\":\"🌃 19:00 - 06:00\",\"callback_data\":\"TL_19_00_06_00\"}],";
                kb += "[{\"text\":\"❌ ปิดระบบออโต้\",\"callback_data\":\"TL_OFF_REQ\"}]";
                kb += "]";

                bot->sendMessageWithInlineKeyboard(chat_id, msg, "Markdown", kb);
            }
            else if (text == "/lighton" || text == "💡 เปิดไฟ")
            {
                if (pm->isPowerSafe())
                {
                    lm->setManualMode(true, true); // Force manual mode ON, turn light ON
                    bot->sendMessage(chat_id, "💡 *เปิดไฟและระงับโหมดออโต้ชั่วคราวแล้ว*", "Markdown");
                }
                else
                {
                    float v = pm->getVoltage();
                    String alertMsg = "⚠️ *ไม่สามารถเปิดไฟได้*\n\n";
                    alertMsg += "🚫 *สาเหตุ:* แรงดันแบตเตอรี่ต่ำกว่าเกณฑ์ปลอดภัย\n";
                    alertMsg += "⚡ *แรงดันปัจจุบัน:* `" + String(v, 2) + "V`\n\n";
                    alertMsg += "ระบบทำการบล็อคคำสั่งเพื่อป้องกันแบตเตอรี่เสียหาย";
                    bot->sendMessage(chat_id, alertMsg, "Markdown");

                    if (m_sysLogger != nullptr)
                    {
                        m_sysLogger->sysLog("TELEGRAM", "Manual light ON rejected: Low Battery (" + String(v, 2) + "V)");
                    }
                }
            }
            else if (text == "/lightoff" || text == "🌑 ปิดไฟ (กลับโหมดออโต้)")
            {
                lm->setManualMode(false, false); // Turn off manual mode, return to Schedule
                bot->sendMessage(chat_id, "🌑 *ปิดไฟและกลับสู่โหมดออโต้แล้ว*", "Markdown");
            }
            else if (text == "/dashboard" || text == "🌐 หน้าเว็บควบคุม")
            {
                String msg = "🌐 *หน้าเว็บควบคุม*\n";
                msg += "⚠️ คลิกลิงก์ด้านล่างเพื่อเข้าสู่หน้าเว็บควบคุม (คุณต้องเชื่อมต่อ WiFi  ชื่อ `T_SOLAR_LED_AP` ก่อนการใช้งาน):\n";
                msg += "👉 http://192.168.4.1\n\n";
                bot->sendMessage(chat_id, msg, "Markdown");
            }
            else if (text == "/update" || text == "🔄 ตรวจสอบอัปเดต")
            {
                bot->sendMessage(chat_id, "⏳ กำลังเชื่อมต่อกับเซิร์ฟเวอร์เพื่อตรวจสอบเวอร์ชันล่าสุด...", "Markdown");
                esp_task_wdt_reset();

                // Fetch latest release tag from GitHub API
                WiFiClientSecure githubClient;
                githubClient.setInsecure();
                githubClient.setTimeout(10000);
                HTTPClient http;
                http.begin(githubClient, SECRET_OTA_UPDATE_API);
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
                String keyboardJson = "[";
                keyboardJson += "[{\"text\":\"✅ ยืนยันการย้อนเวอร์ชัน\", \"callback_data\":\"RB_CONFIRM\"}]";
                keyboardJson += "]";

                bot->sendMessageWithInlineKeyboard(chat_id, "⚠️ *คำเตือน!*\nคุณกำลังจะย้อนเวอร์ชันเฟิร์มแวร์ ระบบจะทำการรีสตาร์ทและอาจหยุดทำงานชั่วคราว ยืนยันหรือไม่?", "Markdown", keyboardJson);
            }
            else
            {
                String keyboardJson = "[";
                keyboardJson += "[\"💡 เปิดไฟ\", \"🌑 ปิดไฟ (กลับโหมดออโต้)\"],";
                keyboardJson += "[\"⏱️ ตั้งเวลาแสง\", \"📊 สถานะระบบ\"],";
                keyboardJson += "[\"🔄 ตรวจสอบอัปเดต\",\"↩️ ย้อนเวอร์ชันอัปเดต\"],";
                keyboardJson += "[\"🌐 หน้าเว็บควบคุม\",\"📝 ดู Log ล่าสุด\"]";
                keyboardJson += "]";
                bot->sendMessageWithReplyKeyboard(chat_id, "👇 เลือกคำสั่งจากปุ่มด้านล่างได้เลย", "", keyboardJson, true);
            }
        }
        esp_task_wdt_reset();
        numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    }
}