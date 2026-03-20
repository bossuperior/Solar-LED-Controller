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

extern Preferences preferences;

void TelegramManager::begin(LogManager *sysLogger)
{
    m_sysLogger = sysLogger;
    m_client.setInsecure();
    if (bot == nullptr) {
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
    if (bot == nullptr) return;
    esp_task_wdt_reset();
    preferences.begin("app_info", false);
    String firmwareVersion = preferences.getString("fw_ver", "v0.0.0-dev");
    preferences.end();
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

                if (data.startsWith("TL_"))
                {
                    int underscore1 = data.indexOf('_');
                    int underscore2 = data.indexOf('_', underscore1 + 1);

                    if (underscore1 != -1 && underscore2 != -1)
                    {
                        int period = data.substring(underscore1 + 1, underscore2).toInt();
                        int percent = data.substring(underscore2 + 1).toInt();

                        lm->setPeriodBrightness(period, percent);

                        if (period == 1)
                        {
                            String keyboardJson = "[[{\"text\":\"40%\",\"callback_data\":\"TL_2_40\"},{\"text\":\"50%\",\"callback_data\":\"TL_2_50\"},{\"text\":\"70%\",\"callback_data\":\"TL_2_70\"}],[{\"text\":\"80%\",\"callback_data\":\"TL_2_80\"},{\"text\":\"100%\",\"callback_data\":\"TL_2_100\"}]]";
                            bot->sendMessageWithInlineKeyboard(chat_id, "✅ บันทึกช่วง 18:00-20:00 เป็น *" + String(percent) + "%* แล้ว\n\n⚙️ *(ขั้นตอน 2/4)*\n🕒 *ช่วงที่ 2: 20:00 - 00:00 น.*\nเลือกระดับความสว่าง:", "Markdown", keyboardJson);
                        }
                        else if (period == 2)
                        {
                            String keyboardJson = "[[{\"text\":\"40%\",\"callback_data\":\"TL_3_40\"},{\"text\":\"50%\",\"callback_data\":\"TL_3_50\"},{\"text\":\"60%\",\"callback_data\":\"TL_3_60\"}],[{\"text\":\"70%\",\"callback_data\":\"TL_3_70\"},{\"text\":\"80%\",\"callback_data\":\"TL_3_80\"}]]";
                            bot->sendMessageWithInlineKeyboard(chat_id, "✅ บันทึกช่วง 20:00-00:00 เป็น *" + String(percent) + "%* แล้ว\n\n⚙️ *(ขั้นตอน 3/4)*\n🕒 *ช่วงที่ 3: 00:00 - 04:00 น.*\nเลือกระดับความสว่าง:", "Markdown", keyboardJson);
                        }
                        else if (period == 3)
                        {
                            String keyboardJson = "[[{\"text\":\"40%\",\"callback_data\":\"TL_4_40\"},{\"text\":\"50%\",\"callback_data\":\"TL_4_50\"},{\"text\":\"60%\",\"callback_data\":\"TL_4_60\"}],[{\"text\":\"70%\",\"callback_data\":\"TL_4_70\"},{\"text\":\"80%\",\"callback_data\":\"TL_4_80\"}]]";
                            bot->sendMessageWithInlineKeyboard(chat_id, "✅ บันทึกช่วง 00:00-04:00 เป็น *" + String(percent) + "%* แล้ว\n\n⚙️ *(ขั้นตอน 4/4)*\n🕒 *ช่วงที่ 4: 04:00 - 06:00 น.*\nเลือกระดับความสว่าง:", "Markdown", keyboardJson);
                        }
                        else if (period == 4)
                        {
                            bot->sendMessage(chat_id, "✅ บันทึกช่วง 04:00-06:00 เป็น *" + String(percent) + "%* แล้ว\n\n🎉 *ตั้งค่าแสงอัตโนมัติครบทั้ง 4 ช่วงเวลาเรียบร้อยครับ!*", "Markdown");
                        }
                    }
                }
                else if (data.startsWith("MAN_"))
                {
                    String valStr = data.substring(4);

                    if (valStr == "AUTO")
                    {
                        lm->setManualMode(false);
                        bot->sendMessage(chat_id, "✅ *โหมดของระบบ: ออโต้*\nระบบกลับไปควบคุมแสงตามเวลาปกติแล้ว", "Markdown");
                    }
                    else
                    {
                        int percent = valStr.toInt();
                        lm->setManualMode(true, percent); // เปิดโหมดแมนนวลที่ X%

                        if (percent == 0)
                        {
                            bot->sendMessage(chat_id, "🌑 *โหมดเปิดปิดไฟเอง: ปิด*\nสั่งปิดไฟแมนนวลเรียบร้อยแล้ว", "Markdown");
                        }
                        else
                        {
                            bot->sendMessage(chat_id, "💡 *โหมดเปิดปิดไฟเอง: เปิด*\nปรับความสว่างเป็น *" + String(percent) + "%* เรียบร้อยแล้ว", "Markdown");
                        }
                    }
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
                int lightPct = 0;
                lm->getBrightness(lightPct);

                int batPct = 0;
                if (v >= 3.40) {
                    batPct = 100;
                } else if (v >= 3.30) {
                    batPct = 90 + ((v - 3.30) / 0.10) * 10; // 3.30V - 3.40V (90% - 100%)
                } else if (v >= 3.20) {
                    batPct = 30 + ((v - 3.20) / 0.10) * 60; // 3.20V - 3.30V (30% - 90%)
                } else if (v >= 3.10) {
                    batPct = 5 + ((v - 3.10) / 0.10) * 25;  // 3.10V - 3.20V (5% - 30%)
                } else if (v >= 3.00) {
                    batPct = ((v - 3.00) / 0.10) * 5;       // 3.00V - 3.10V (0% - 5%)
                } else {
                    batPct = 0;
                }
                batPct = constrain(batPct, 0, 100); 

                String msg = "📊 *รายงานสถานะระบบ*\n\n";
                msg += "⚡ *พลังงาน:* " + String(v, 2) + "V (" + String(batPct) + "%)\n";
                msg += "💡 *แสง:* " + String(lightPct) + "%\n";
                msg += "🌡️ *อุณหภูมิ:* LED " + String(tLed, 1) + "°C | Buck " + String(tBuck, 1) + "°C\n";
                msg += "🌀 *พัดลม:* " + String(fanSpeed > 0 ? "เปิด" : "ปิด") + " | ความเร็ว: " + String(fanSpeed) + "%\n";
                msg += "🔄 *เวอร์ชันเฟิร์มแวร์:* " + firmwareVersion + "\n";

                bot->sendMessage(chat_id, msg, "Markdown");
            }
            else if (text == "/timelight" || text == "⏱️ ตั้งเวลาแสง")
            {
                // ส่งเฉพาะคำถามของช่วงที่ 1 (18:00 - 20:00)
                String keyboardJson = "[";
                keyboardJson += "[{\"text\":\"40%\",\"callback_data\":\"TL_1_40\"},{\"text\":\"50%\",\"callback_data\":\"TL_1_50\"},{\"text\":\"70%\",\"callback_data\":\"TL_1_70\"}],";
                keyboardJson += "[{\"text\":\"80%\",\"callback_data\":\"TL_1_80\"},{\"text\":\"100%\",\"callback_data\":\"TL_1_100\"}]";
                keyboardJson += "]";

                bot->sendMessageWithInlineKeyboard(chat_id, "⚙️ *ตั้งค่าแสง (ขั้นตอน 1/4)*\n\n🕒 *ช่วงที่ 1: 18:00 - 20:00 น.*\nเลือกระดับความสว่างที่ต้องการ:", "Markdown", keyboardJson);
            }
            else if (text == "/lighton" || text == "💡 เปิดไฟ (แมนนวล)")
            {
                lm->setManualMode(true, 100);
                String keyboardJson = "[";
                // แถว 1: 20, 40, 60
                keyboardJson += "[{\"text\":\"20%\",\"callback_data\":\"MAN_20\"},{\"text\":\"40%\",\"callback_data\":\"MAN_40\"},{\"text\":\"60%\",\"callback_data\":\"MAN_60\"}],";
                // แถว 2: 80, 100
                keyboardJson += "[{\"text\":\"80%\",\"callback_data\":\"MAN_80\"},{\"text\":\"100%\",\"callback_data\":\"MAN_100\"}]";
                keyboardJson += "]";

                bot->sendMessageWithInlineKeyboard(chat_id, "💡 *เปิดไฟ 100% แล้ว*\nสามารถปรับลดระดับความสว่างได้:", "Markdown", keyboardJson);
            }
            else if (text == "/lightoff" || text == "🌑 ปิดไฟ (กลับโหมด AUTO)")
            {
                lm->setManualMode(false, 0);
                bot->sendMessage(chat_id, "💡 ปิดไฟแล้วและกลับสู่โหมดอัตโนมัติ", "Markdown");
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

                //Fetch latest release tag from GitHub API
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
                keyboardJson += "[\"💡 เปิดไฟ (แมนนวล)\", \"🌑 ปิดไฟ (กลับโหมด AUTO)\"],";
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