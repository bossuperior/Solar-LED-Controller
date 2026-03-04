#include "TelegramManager.h"
#include "LightManager.h"

void TelegramManager::begin(LogManager *sysLogger)
{
    m_sysLogger = sysLogger;
    client.setInsecure();
    bot = new UniversalTelegramBot(TELEGRAM_BOT_TOKEN, client);
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

    bot->sendMessage(TELEGRAM_CHAT_ID, text, "Markdown");
}

void TelegramManager::checkMessages(PowerManager *pm, TempManager *tm, FanManager *fm, LightManager *lm)
{
    int numNewMessages = bot->getUpdates(bot->last_message_received + 1);

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
            }
            else if (text == "/status" || text == "📊 สถานะระบบ")
            {
                float v = pm->getVoltage();
                float c = pm->getCurrent();
                float tLed = tm->getLedTemp();
                float tBuck = tm->getBuckTemp();
                int fanSpeed = fm->getFanSpeed();

                int lightPct = 0;
                lm->getBrightness(lightPct);

                String msg = "📊 *รายงานสถานะระบบ*\n\n";
                msg += "⚡ *พลังงาน:* " + String(v, 2) + "V | " + String(c, 2) + "A\n";
                msg += "💡 *แสง:* " + String(lightPct) + "%\n";
                msg += "🌡️ *อุณหภูมิ:* LED " + String(tLed, 1) + "°C | Buck " + String(tBuck, 1) + "°C\n";
                msg += "🌀 *พัดลม:* " + String(fanSpeed > 0 ? "เปิด" : "ปิด") + "\n";

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
                keyboardJson += "[{\"text\":\"80%\",\"callback_data\":\"MAN_80\"},{\"text\":\"100%\",\"callback_data\":\"MAN_100\"}],";
                keyboardJson += "]";

                bot->sendMessageWithInlineKeyboard(chat_id, "💡 *เปิดไฟ 100% แล้ว*\nสามารถปรับลดระดับความสว่างได้:", "Markdown", keyboardJson);
            }
            else if (text == "/lightoff" || text == "🌑 ปิดไฟ (กลับโหมด AUTO)")
            {
                lm->setManualMode(false, 0);
                bot->sendMessage(chat_id, "💡 ปิดไฟแล้วและกลับสู่โหมดอัตโนมัติ", "Markdown");
            }
            else
            {
                String keyboardJson = "[";
                keyboardJson += "[\"💡 เปิดไฟ (แมนนวล)\", \"🌑 ปิดไฟ (กลับโหมด AUTO)\"],";
                keyboardJson += "[\"⏱️ ตั้งเวลาแสง\", \"📊 สถานะระบบ\"]";
                keyboardJson += "]";

                bot->sendMessageWithReplyKeyboard(chat_id, "👇 เลือกคำสั่งจากปุ่มด้านล่างได้เลย", "", keyboardJson, true);
            }
        }
        numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    }
}