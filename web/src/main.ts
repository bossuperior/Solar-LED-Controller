import './style.css'

const getBatteryPercent = (volts: number): number => {
    const vMin = 2.80; 
    const vMax = 3.60; 
    if (volts >= vMax) return 100;
    if (volts <= vMin) return 0;
    return Math.round(((volts - vMin) / (vMax - vMin)) * 100);
};

const showToast = (msg: string) => {
    const toast = document.getElementById('toast')!;
    toast.innerText = `✅ ${msg}`;
    toast.style.bottom = '40px';
    setTimeout(() => { toast.style.bottom = '-100px'; }, 2500);
}

(window as any).sendCmd = async (path: string) => {
    try {
        await fetch(path);
        showToast("สั่งงานเรียบร้อย!");
    } catch (e) { console.error(e); }
}

(window as any).setSch = async (p: number, v: number, btn: HTMLButtonElement) => {
    try {
        const res = await fetch(`/set_sch?p=${p}&v=${v}`);
        if (res.ok) {
            const btns = document.querySelectorAll(`#g${p} button`);
            btns.forEach(b => b.classList.remove('bg-blue-600', 'ring-2', 'ring-white'));
            btn.classList.add('bg-blue-600', 'ring-2', 'ring-white');
            showToast(`บันทึกช่วงเวลา ${p} เป็น ${v}%`);
        }
    } catch (e) { console.error(e); }
}

const createPresetButtons = (containerId: string, period: number, values: number[]) => {
    const container = document.getElementById(containerId)!;
    values.forEach(val => {
        const btn = document.createElement('button');
        btn.className = "flex-none px-5 py-2 bg-slate-800 border border-slate-700 rounded-xl font-bold text-sm transition-all active:scale-90";
        btn.innerText = `${val}%`;
        btn.onclick = () => (window as any).setSch(period, val, btn);
        container.appendChild(btn);
    });
}

document.addEventListener('DOMContentLoaded', () => {
    createPresetButtons('g1', 1, [40, 50, 70, 80, 100]);
    createPresetButtons('g2', 2, [40, 50, 70, 80, 100]);
    createPresetButtons('g3', 3, [40, 50, 60, 70, 80]);
    createPresetButtons('g4', 4, [40, 50, 60, 70, 80]);

    // auto-refresh status every 3 seconds
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
                fanEl.innerText = "ON 🌀";
                fanEl.className = "text-2xl font-black text-blue-400 mt-1";
            } else {
                fanEl.innerText = "OFF 🛑";
                fanEl.className = "text-2xl font-black text-slate-500 mt-1";
            }

            // 4. Light Intensity
            document.getElementById('light-status')!.innerText = `${data.light} %`;

        } catch (e) { console.error("ออฟไลน์หรือเชื่อมต่อไม่ได้"); }
    }, 3000);
});