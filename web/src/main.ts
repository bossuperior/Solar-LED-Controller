declare global {
    interface Window {
        sendCmd: (endpoint: string) => Promise<void>;
        saveSchedule: (isActive: number | boolean) => Promise<void>;
    }
}

interface SystemStatus {
    volt: number;
    temp_buck: number;
    fan_on: boolean;
    light: string;
    firmware?: string;
    uptime_sec?: number;
    alert?: string;
    s_h: number;
    s_m: number;
    e_h: number;
    e_m: number;
    sch_active: boolean;
}

let isFirstLoad = true;
let toastTimeout: number | undefined;

async function fetchStatus(): Promise<void> {
    try {
        const response = await fetch('/status');
        if (!response.ok) throw new Error("Network response error");
        
        const data: SystemStatus = await response.json();
        if (isFirstLoad) {
            const startInput = document.getElementById('startTime') as HTMLInputElement | null;
            const endInput = document.getElementById('endTime') as HTMLInputElement | null;

            if (startInput && endInput) {
                const formatTime = (h: number, m: number) => 
                    `${h.toString().padStart(2, '0')}:${m.toString().padStart(2, '0')}`;

                startInput.value = formatTime(data.s_h, data.s_m);
                endInput.value = formatTime(data.e_h, data.e_m);
                
                isFirstLoad = false;
            }
        }

        const vEl = document.getElementById('v');
        const batPctEl = document.getElementById('bat-pct');
        if (vEl) vEl.innerText = data.volt.toFixed(2) + ' V';
        
        let pct = 0; 
        const v = data.volt;
        if (v >= 3.45) pct = 100;
        else if (v >= 3.35) pct = 90 + ((v - 3.35) / 0.10) * 10;
        else if (v >= 3.20) pct = 30 + ((v - 3.20) / 0.15) * 60;
        else if (v >= 2.80) pct = ((v - 2.80) / 0.40) * 30;
        pct = Math.min(Math.max(Math.round(pct), 0), 100); 
        
        if (batPctEl) batPctEl.innerText = `(${pct}%)`;
        
        const tBuckEl = document.getElementById('t-buck');
        if (tBuckEl) tBuckEl.innerHTML = `${data.temp_buck.toFixed(1)} °C`;

        const fanEl = document.getElementById('fan-status');
        if (fanEl) {
            if (data.fan_on) {
                fanEl.innerText = "เปิด 🌀";
                fanEl.className = "text-2xl md:text-3xl font-black text-emerald-400 mt-2";
            } else {
                fanEl.innerText = "ปิด 🛑";
                fanEl.className = "text-2xl md:text-3xl font-black text-slate-500 mt-2";
            }
        }

        const lightEl = document.getElementById('light-status');
        if (lightEl) {
            lightEl.innerText = data.light;
            if (data.light === "เปิด(สว่างสุด)") {
                lightEl.className = "text-2xl md:text-3xl font-black text-amber-400 mt-2 drop-shadow-[0_0_8px_rgba(251,191,36,0.8)]";
            } else if (data.light === "เปิด(ลดความสว่าง)") {
                lightEl.className = "text-2xl md:text-3xl font-black text-orange-500 mt-2";
            } else if (data.light === "ปิดไฟ") {
                lightEl.className = "text-2xl md:text-3xl font-black text-slate-500 mt-2";
            } else {
                lightEl.className = "text-2xl md:text-3xl font-black text-slate-300 mt-2";
            }
        }

        const fwEl = document.getElementById('fw-ver');
        if (fwEl && data.firmware) fwEl.innerText = `Firmware ${data.firmware}`;

        const uptEl = document.getElementById('sys-upt');
        if (uptEl && data.uptime_sec !== undefined) {
            const u = data.uptime_sec;
            const d = Math.floor(u / 86400);
            const h = Math.floor((u % 86400) / 3600);
            const m = Math.floor((u % 3600) / 60);
            uptEl.innerText = `เวลาการทำงานระบบ: ${d} วัน ${h} ชม. ${m} นาที`;
        }

        if (data.alert && data.alert.trim() !== "") {
            showToast(data.alert);
        }

    } catch (error) {
        console.error('Error fetching status:', error);
    }
}

window.sendCmd = async function(endpoint: string): Promise<void> {
    if (endpoint === '/log') {
        window.open('/log', '_blank');
        return;
    }

    try {
        const response = await fetch(endpoint, { method: 'POST' });
        if (response.ok) {
            showToast("ส่งคำสั่งสำเร็จ! 🚀");
            fetchStatus();
        } else {
            showToast("บอร์ดปฏิเสธคำสั่ง (แบตอาจจะต่ำ) ⚠️");
        }
    } catch (error) {
        showToast("เชื่อมต่อผิดพลาด ❌");
    }
};

window.saveSchedule = async function(isActive: number | boolean): Promise<void> {
    const activeStr = isActive ? 'true' : 'false';
    let url = `/set_sch?active=${activeStr}`;
    
    if (isActive) {
        const startEl = document.getElementById('startTime') as HTMLInputElement | null;
        const endEl = document.getElementById('endTime') as HTMLInputElement | null;
        
        const start = startEl?.value;
        const end = endEl?.value;
        
        if (!start || !end) {
            showToast("กรุณากรอกเวลาให้ครบ! ⚠️");
            return;
        }

        const [sH, sM] = start.split(':');
        const [eH, eM] = end.split(':');
        url += `&sh=${sH}&sm=${sM}&eh=${eH}&em=${eM}`;
    } else {
        url += '&sh=0&sm=0&eh=0&em=0'; 
    }

    try {
        const response = await fetch(url, { method: 'POST' });
        if (response.ok) {
            showToast(isActive ? "บันทึกเวลาเปิด-ปิดอัตโนมัติแล้ว 💾" : "ปิดระบบอัตโนมัติแล้ว ❌");
            isFirstLoad = true;
            fetchStatus();
        } else {
            showToast("บันทึกไม่สำเร็จ ⚠️");
        }
    } catch (error) {
        showToast("เชื่อมต่อผิดพลาด ❌");
    }
};

function showToast(message: string): void {
    const toast = document.getElementById('toast');
    if (toast) {
        toast.innerText = message;
        toast.style.bottom = '20px'; 
        
        if (toastTimeout) {
            clearTimeout(toastTimeout);
        }

        toastTimeout = window.setTimeout(() => {
            toast.style.bottom = '-100px'; 
        }, 5000); 
    }
}

fetchStatus();
setInterval(fetchStatus, 3000);