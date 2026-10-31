// Bu dosya, güvenli gezinme uyarı sayfasının davranışını yönetir: adres çubuğundaki "hedef" sorgu parametresinden şüpheli adresi okur, "Geri Dön"/"Yine de Git" tıklamalarını işler.
// Sertifika hatası panelinin aksine bu sayfa, normal bir sekme içinde (ayrı bir WebView2 denetleyicisi olmadan) gösterilir, bu yüzden native taraftan veri beklemez, kendi adresindeki sorgu parametresini doğrudan okur.

const hedefAdres = new URLSearchParams(window.location.search).get("hedef") || ""; // BrowserWindow.cpp tarafından yüzde kodlamasıyla eklenen hedef adres, tarayıcının kendi URLSearchParams'ı ile çözülür

document.getElementById("sertifika-adres").textContent = hedefAdres;

document.getElementById("guvenli-gezinme-geri-don-butonu").addEventListener("click", function () {
    window.history.length > 1 ? window.history.back() : window.location.replace("https://tarayici.local/newtab.html"); // Geri gidilecek bir sayfa varsa oraya, yoksa hızlı erişim sayfasına dönülür
});

document.getElementById("guvenli-gezinme-yine-de-git-butonu").addEventListener("click", function () {
    if (!hedefAdres) { return; }
    window.chrome.webview.postMessage("safebrowsing_bypass|" + hedefAdres); // Native tarafa, bu TEK adrese bir kerelik izin verilmesi bildirilir, ardından aynı adrese tekrar gezinilir
});

document.addEventListener("keydown", function (olay) {
    if (olay.key === "Escape") { // Escape, "Geri Dön" (güvenli taraf) kararıyla aynı davranır
        document.getElementById("guvenli-gezinme-geri-don-butonu").click();
    }
});
