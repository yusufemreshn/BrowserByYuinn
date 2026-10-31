// Bu dosya, adres çubuğu öneri panelinin davranışını yönetir: native taraftan gelen eşleşen adresleri gösterir, bir satıra tıklanınca native tarafa bildirir.
// Bu panel kendi ayrı WebView2 denetleyicisinde çalışır, açılıp kapanma ve konumlanma işini native taraf (BrowserWindow.cpp) yönetir, bu dosya sadece panelin İÇİNDEKİ davranıştan sorumludur.

const oneriListesi = document.getElementById("oneri-listesi"); // Önerilerin ekleneceği kapsayıcı

window.chrome.webview.addEventListener("message", function (olay) {
    let veri;
    try {
        veri = JSON.parse(olay.data);
    } catch (hata) {
        return; // Eğer mesaj geçerli bir JSON değilse, güvenli bir şekilde yok sayılır
    }

    if (veri.type !== "addresssuggestions") { return; } // Bu panel sadece kendi veri türüyle ilgilenir

    oneriListesi.innerHTML = ""; // Önceki içerik temizlenir
    veri.entries.forEach(function (kayit) { // Her eşleşen kayıt için bir satır oluşturulur
        const satir = document.createElement("div");
        satir.className = "oneri-satiri";

        if (kayit.calc) { // Bu girdi bir hesap makinesi/birim çevirici sonucuysa, normal bir adres kaydı gibi davranmaz
            const sonucOgesi = document.createElement("div");
            sonucOgesi.className = "oneri-baslik";
            sonucOgesi.textContent = kayit.title;
            satir.appendChild(sonucOgesi);
            satir.title = "Sonucu kopyalamak için tıklayın";
            satir.addEventListener("click", function () {
                const esittirKonumu = kayit.title.lastIndexOf("=");
                const kopyalanacak = esittirKonumu === -1 ? kayit.title : kayit.title.substring(esittirKonumu + 1).trim();
                navigator.clipboard.writeText(kopyalanacak); // Sadece sonuç kısmı (= işaretinden sonrası) panoya kopyalanır
            });
            oneriListesi.appendChild(satir);
            return;
        }

        const baslikOgesi = document.createElement("div");
        baslikOgesi.className = "oneri-baslik";
        baslikOgesi.textContent = kayit.title || kayit.address;

        const adresOgesi = document.createElement("div");
        adresOgesi.className = "oneri-adres";
        adresOgesi.textContent = kayit.address;

        satir.appendChild(baslikOgesi);
        satir.appendChild(adresOgesi);
        satir.addEventListener("click", function () {
            window.chrome.webview.postMessage("addresssuggestion_choice|" + kayit.address);
        });
        oneriListesi.appendChild(satir);
    });
});
