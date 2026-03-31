import './style.css'

// 1. Updated to match the LiFePO4 1S (3.2V Nominal) curve
const getBatteryPercent = (v: number): number => {
    let batPct = 0;
    if (v >= 3.45) batPct = 100;
    else if (v >= 3.35) batPct = 90 + ((v - 3.35) / 0.10) * 10;
    else if (v >= 3.25) batPct = 70 + ((v - 3.25) / 0.10) * 20;
    else if (v >= 3.20) batPct = 30 + ((v - 3.20) / 0.05) * 40;
    else if (v >= 3.10) batPct = 10 + ((v - 3.10) / 0.10) * 20;
    else if (v >= 2.80) batPct = ((v - 2.80) / 0.30) * 10;
    else batPct = 0;
    
    return Math.max(0, Math.min(100, Math.round(batPct)));
};

const showToast = (msg: string) => {
    const toast = document.getElementById('toast')!;
    toast.innerText = `✅ ${msg}`;
    toast.style.bottom = '40px';
    setTimeout(() => { toast.style.bottom = '-100px'; }, 2500);
}

// 2. Button commands for Light ON / Light OFF
(window as any).sendCmd = async (path: string) => {
    try {
        await fetch(path);
        showToast("สั่งงานเรียบร้อย!");
    } catch (e) { console.error(e); }
}

// 3. New Schedule Save Function
(window as any).saveSchedule = async (isActive: number) => {
    const startTime = (document.getElementById('startTime') as HTMLInputElement).value;
    const endTime = (document.getElementById('endTime') as HTMLInputElement).value;

    // Only force the user to fill out the time if they are trying to ENABLE the schedule
    if (isActive === 1 && (!startTime || !endTime)) {
        alert("กรุณาระบุเวลาให้ครบถ้วน"); 
        return;
    }

    // Safe fallback for the split logic if they disable without entering a time
    const [sh, sm] = startTime ? startTime.split(':') : ["0", "0"];
    const [eh, em] = endTime ? endTime.split(':') : ["0", "0"];

    try {
        const res = await fetch(`/set_sch?sh=${sh}&sm=${sm}&eh=${eh}&em=${em}&active=${isActive}`);
        if (res.ok) {
            if (isActive === 1) {
                showToast("เปิดระบบออโต้และบันทึกเวลาสำเร็จ!"); // Enabled msg
            } else {
                showToast("ปิดระบบออโต้เรียบร้อย!"); // Disabled msg
            }
        } else {
            alert("ระบบมีปัญหาในการสั่งงาน");
        }
    } catch (e) { 
        console.error(e); 
    }
}

document.addEventListener('DOMContentLoaded', () => {
    // Auto-refresh status every 3 seconds
    setInterval(async () => {
        try {
            const res = await fetch('/status');
            const data = await res.json();
            
            // 1. Battery Voltage & Percent
            document.getElementById('v')!.innerText = `${data.volt.toFixed(1)} V`;
            document.getElementById('bat-pct')!.innerText = `(${getBatteryPercent(data.volt)}%)`;
            
            // 2. Temperatures
            document.getElementById('t-buck')!.innerText = `${data.temp_buck.toFixed(1)} °C (วงจรลดแรงดัน)`;
            document.getElementById('t-led')!.innerText = `${data.temp_led.toFixed(1)} °C (ใต้แผงแอลอีดี)`;
            
            // 3. Fan Status
            const fanEl = document.getElementById('fan-status')!;
            if (data.fan_on) {
                fanEl.innerText = "เปิด 🌀";
                fanEl.className = "text-2xl font-black text-blue-400 mt-1";
            } else {
                fanEl.innerText = "ปิด 🛑";
                fanEl.className = "text-2xl font-black text-slate-500 mt-1";
            }

            // 4. Light Status
            document.getElementById('light-status')!.innerText = data.light;

        } catch (e) { 
            console.error("ออฟไลน์หรือเชื่อมต่อไม่ได้ (Offline or unreachable)"); 
        }
    }, 3000);
});