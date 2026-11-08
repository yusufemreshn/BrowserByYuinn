// Bu dosya, adres çubuğu öneri panelinin davranışını yönetir: native taraftan gelen eşleşen adresleri gösterir, bir satıra tıklanınca native tarafa bildirir.
// Bu panel kendi ayrı WebView2 denetleyicisinde çalışır, açılıp kapanma ve konumlanma işini native taraf (BrowserWindow.cpp) yönetir, bu dosya sadece panelin İÇİNDEKİ davranıştan sorumludur.

const oneriListesi = document.getElementById("oneri-listesi"); // Önerilerin ekleneceği kapsayıcı
let secilebilenSatirlar = []; // Hesap sonucu satırı HARİÇ, klavyeyle gezinilebilen satırlar, native taraftaki index'le aynı sırada tutulur

// Bu fonksiyon, bir öğenin içine, sorguyla eşleşen kısmı <mark> ile vurgulanmış şekilde metin ekler
// innerHTML yerine metin düğümleri kullanılır, böylece geçmiş/yer imi başlıklarındaki herhangi bir karakter güvenli şekilde işlenir
function vurgulananMetniEkle(hedefOge, metin, sorguKucuk) {
    hedefOge.textContent = ""; // Önceki içerik temizlenir
    if (!sorguKucuk) { // Sorgu yoksa (örn. çok eski bir mesaj), vurgusuz düz metin eklenir
        hedefOge.appendChild(document.createTextNode(metin));
        return;
    }
    const konum = metin.toLowerCase().indexOf(sorguKucuk);
    if (konum === -1) { // Bu metinde sorgu geçmiyorsa (örn. diğer alanda eşleşmiş), vurgusuz eklenir
        hedefOge.appendChild(document.createTextNode(metin));
        return;
    }
    if (konum > 0) {
        hedefOge.appendChild(document.createTextNode(metin.substring(0, konum)));
    }
    const vurgu = document.createElement("mark");
    vurgu.className = "oneri-eslesme";
    vurgu.textContent = metin.substring(konum, konum + sorguKucuk.length);
    hedefOge.appendChild(vurgu);
    if (konum + sorguKucuk.length < metin.length) {
        hedefOge.appendChild(document.createTextNode(metin.substring(konum + sorguKucuk.length)));
    }
}

window.chrome.webview.addEventListener("message", function (olay) {
    let veri;
    try {
        veri = JSON.parse(olay.data);
    } catch (hata) {
        return; // Eğer mesaj geçerli bir JSON değilse, güvenli bir şekilde yok sayılır
    }

    if (veri.type === "addresssuggestions_select") { // Native taraf, ok tuşlarıyla klavye seçimini değiştirdiğini bildiriyorsa, sadece ilgili satırın görünümü güncellenir
        secilebilenSatirlar.forEach(function (satir, indeks) {
            satir.classList.toggle("secili", indeks === veri.index);
        });
        if (veri.index >= 0 && secilebilenSatirlar[veri.index]) {
            secilebilenSatirlar[veri.index].scrollIntoView({ block: "nearest" }); // Panel kaydırılabilir olduğundan, seçili satır görünüm dışındaysa içeri kaydırılır
        }
        return;
    }

    if (veri.type !== "addresssuggestions") { return; } // Bu panel başka hiçbir mesaj türüyle ilgilenmez

    const sorguKucuk = (veri.query || "").toLowerCase(); // Vurgulama için, sorgunun küçük harfli hali hazırlanır
    oneriListesi.innerHTML = ""; // Önceki içerik temizlenir
    secilebilenSatirlar = []; // Yeni liste geldiğinde, önceki klavye-gezinme listesi geçersizdir

    veri.entries.forEach(function (kayit) { // Her eşleşen kayıt için bir satır oluşturulur
        const satir = document.createElement("div");
        satir.className = "oneri-satiri";

        if (kayit.calc) { // Bu girdi bir hesap makinesi/birim çevirici sonucuysa, normal bir adres kaydı gibi davranmaz, klavyeyle de seçilemez
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

        const simgeOgesi = document.createElement("div"); // Satırın solundaki küçük küre ikonu, bu önerinin bir web adresi olduğunu görsel olarak belirtir
        simgeOgesi.className = "oneri-simge";
        simgeOgesi.innerHTML = '<svg viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.3"><circle cx="8" cy="8" r="6.3"/><path d="M1.7 8h12.6M8 1.7c2.1 2.3 2.1 10.3 0 12.6M8 1.7c-2.1 2.3-2.1 10.3 0 12.6"/></svg>';

        const metinlerOgesi = document.createElement("div");
        metinlerOgesi.className = "oneri-metinler";

        const baslikOgesi = document.createElement("div");
        baslikOgesi.className = "oneri-baslik";
        vurgulananMetniEkle(baslikOgesi, kayit.title || kayit.address, sorguKucuk);

        const adresOgesi = document.createElement("div");
        adresOgesi.className = "oneri-adres";
        vurgulananMetniEkle(adresOgesi, kayit.address, sorguKucuk);

        metinlerOgesi.appendChild(baslikOgesi);
        metinlerOgesi.appendChild(adresOgesi);
        satir.appendChild(simgeOgesi);
        satir.appendChild(metinlerOgesi);
        satir.addEventListener("click", function () {
            window.chrome.webview.postMessage("addresssuggestion_choice|" + kayit.address);
        });
        oneriListesi.appendChild(satir);
        secilebilenSatirlar.push(satir); // Bu satır, native taraftaki m_oneriSeciliAdresler ile aynı sırada, klavyeyle gezinilebilir listeye eklenir
    });
});
