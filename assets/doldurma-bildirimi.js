// Bu dosya, otomatik doldurma panelinin davranışını yönetir: native taraftan gelen kayıtlı kullanıcı adı önerilerini listeler, bir seçim yapılınca native tarafa bildirir.
// Bu panel kendi ayrı WebView2 denetleyicisinde çalışır, açılıp kapanma ve konumlanma işini native taraf (BrowserWindow.cpp) yönetir, bu dosya sadece panelin İÇİNDEKİ davranıştan sorumludur.

const doldurmaListesi = document.getElementById("doldurma-listesi"); // Önerilerin ekleneceği kapsayıcı

// Native taraftan bir mesaj geldiğinde (gösterilecek öneriler bildirildiğinde) bu fonksiyon çalışır
window.chrome.webview.addEventListener("message", function (olay) {
    let veri;
    try {
        veri = JSON.parse(olay.data);
    } catch (hata) {
        return; // Eğer mesaj geçerli bir JSON değilse, güvenli bir şekilde yok sayılır
    }

    if (veri.type !== "autofillsuggestions") { return; } // Bu panel sadece kendi veri türüyle ilgilenir

    doldurmaListesi.innerHTML = ""; // Önceki içerik temizlenir
    veri.usernames.forEach(function (kullaniciAdi) { // Her kayıtlı kullanıcı adı için bir öneri satırı oluşturulur
        const satir = document.createElement("div");
        satir.className = "doldurma-oneri-satiri";
        satir.textContent = "🔑 " + kullaniciAdi;
        satir.addEventListener("click", function () {
            window.chrome.webview.postMessage("autofill_choice|" + kullaniciAdi); // Seçilen kullanıcı adı, native tarafa bildirilir, Windows kimlik doğrulaması orada istenir
        });
        doldurmaListesi.appendChild(satir);
    });
});

document.addEventListener("keydown", function (olay) {
    if (olay.key === "Escape") { // Kullanıcı Escape'e basarsa, panel kapatılır
        window.chrome.webview.postMessage("autofill_close");
    }
});
