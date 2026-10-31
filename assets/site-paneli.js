// Bu dosya, site bilgisi/izinleri panelinin davranışını yönetir: native taraftan gelen site bilgisini ve izin listesini gösterir, "Kaldır" tıklamalarını native tarafa bildirir.
// Bu panel kendi ayrı WebView2 denetleyicisinde çalıştığı için, açılıp kapanma ve konumlanma işini native taraf (BrowserWindow.cpp) yönetir, bu dosya sadece panelin İÇİNDEKİ davranıştan sorumludur.

const sitePaneliAdres = document.getElementById("site-paneli-adres"); // Sitenin alan adının gösterileceği öğe
const sitePaneliDurum = document.getElementById("site-paneli-durum"); // Güvenlik durumu metninin gösterileceği öğe
const sitePaneliSertifika = document.getElementById("site-paneli-sertifika"); // Sertifika detaylarının gösterileceği öğe
const sitePaneliIzinYok = document.getElementById("site-paneli-izin-yok"); // "Hiç izin kaydı yok" metni
const sitePaneliIzinlerListesi = document.getElementById("site-paneli-izinler-listesi"); // İzin satırlarının ekleneceği kapsayıcı

// Her izin türü için, kullanıcıya gösterilecek doğal dilde bir ad, izin bildirim panelindeki (izin-bildirimi.js) türlerle aynıdır
const izinTuruAdlari = {
    konum: "Konum",
    kamera: "Kamera",
    mikrofon: "Mikrofon",
    bildirim: "Bildirimler",
    otomatikoynatma: "Otomatik Oynatma"
};

// Bu fonksiyon, bir sitenin tam kaynak adresinden (örneğin "https://example.com/sayfa"), kullanıcıya gösterilecek sade alan adını çıkarır
function kaynaktanAlanAdiCikar(kaynak) {
    try {
        return new URL(kaynak).hostname || kaynak; // Tarayıcının kendi URL ayrıştırıcısı kullanılır, alan adı boşsa (örneğin "about:blank") kaynak olduğu gibi gösterilir
    } catch (hata) {
        return kaynak; // Ayrıştırılamazsa, kaynak olduğu gibi gösterilir
    }
}

// Native taraftan bir mesaj geldiğinde (panel açıldığında veya bir izin kaldırıldıktan sonra güncellenince) bu fonksiyon çalışır
window.chrome.webview.addEventListener("message", function (olay) {
    let veri; // Gelen mesajın JSON'dan çözülmüş halini tutacak değişken
    try {
        veri = JSON.parse(olay.data); // Native tarafın gönderdiği JSON metni, bir JavaScript nesnesine çevrilir
    } catch (hata) {
        return; // Eğer mesaj geçerli bir JSON değilse, güvenli bir şekilde yok sayılır
    }

    if (veri.type !== "sitebilgisi") { return; } // Bu panel sadece kendi veri türüyle ilgilenir

    sitePaneliAdres.textContent = veri.internal ? "Tarayıcının dahili sayfası" : kaynaktanAlanAdiCikar(veri.origin); // Dahili sayfalarda sahte "tarayici.local" adresi yerine anlaşılır bir metin gösterilir

    sitePaneliDurum.classList.toggle("guvensiz", !veri.secure); // Güvenli değilse, durum metni uyarı rengiyle gösterilir
    if (veri.internal) {
        sitePaneliDurum.textContent = "Bu, programın kendi güvenli dahili sayfasıdır.";
    } else if (veri.secure) {
        sitePaneliDurum.textContent = "🔒 Bağlantı güvenli (HTTPS).";
    } else {
        sitePaneliDurum.textContent = "⚠ Bağlantı güvenli değil (HTTP), bu sayfada girilen bilgiler şifrelenmeden gönderilebilir.";
    }

    if (veri.certificate) { // Sertifika detayları geldiyse (sadece hatasız HTTPS bağlantılarında, ve native tarafın CDP olayını yakalamasından kısa bir süre sonra), gösterilir
        // ÖNEMLİ: bu alanlar (subject/issuer/protocol) ziyaret edilen sitenin GERÇEK TLS sertifikasından gelir, yani saldırganın kontrolünde olabilir
        // Bu panel, toolbar ile aynı ayrıcalıklı komut kanalını paylaştığı için, bu alanlar innerHTML ile DEĞİL, textContent ile (HTML olarak yorumlanmadan) yazılmalıdır
        sitePaneliSertifika.innerHTML = "";
        const satirEkle = function (etiket, deger) {
            const satir = document.createElement("div");
            const kalinEtiket = document.createElement("strong");
            kalinEtiket.textContent = etiket + ": ";
            satir.appendChild(kalinEtiket);
            satir.appendChild(document.createTextNode(deger || "—"));
            sitePaneliSertifika.appendChild(satir);
        };
        satirEkle("Sertifika konusu", veri.certificate.subject);
        satirEkle("Yayınlayan", veri.certificate.issuer);
        satirEkle("Protokol", veri.certificate.protocol);
        sitePaneliSertifika.classList.remove("gizli");
    } else {
        sitePaneliSertifika.classList.add("gizli");
    }

    sitePaneliIzinlerListesi.innerHTML = ""; // Önceki liste tamamen temizlenir, yeniden çizilir
    if (!veri.permissions || veri.permissions.length === 0) { // Bu site için hiç izin kaydı yoksa
        sitePaneliIzinYok.classList.remove("gizli"); // Bilgilendirme metni gösterilir
    } else {
        sitePaneliIzinYok.classList.add("gizli"); // Bilgilendirme metni gizlenir, gerçek liste gösterilecek
        veri.permissions.forEach(function (kayit) { // Her izin kaydı için bir satır oluşturulur
            const satir = document.createElement("div");
            satir.className = "site-paneli-izin-satiri";

            const solKisim = document.createElement("span");
            const turAdi = izinTuruAdlari[kayit.tur] || kayit.tur; // Türün doğal dildeki adı bulunur, tanınmayan bir türse olduğu gibi gösterilir
            const kararMetni = kayit.izinVerildi ? "İzin verildi" : "Engellendi"; // Kararın okunabilir hali
            solKisim.textContent = turAdi;
            const kararEtiketi = document.createElement("span");
            kararEtiketi.className = "site-paneli-izin-karari";
            kararEtiketi.textContent = "· " + kararMetni;
            solKisim.appendChild(kararEtiketi);

            const kaldirButonu = document.createElement("button");
            kaldirButonu.className = "site-paneli-kaldir-butonu";
            kaldirButonu.textContent = "Kaldır";
            kaldirButonu.addEventListener("click", function () {
                window.chrome.webview.postMessage("site_paneli_izin_kaldir|" + kayit.tur); // Native tarafa, bu izin kaydının kaldırılması komutu gönderilir, hangi SİTEYE ait olduğunu native taraf kendisi (açık olan panelin kaynağından) bilir
            });

            satir.appendChild(solKisim);
            satir.appendChild(kaldirButonu);
            sitePaneliIzinlerListesi.appendChild(satir);
        });
    }
});

document.addEventListener("keydown", function (olay) {
    if (olay.key === "Escape") { // Kullanıcı Escape'e basarsa, panel kapatılır
        window.chrome.webview.postMessage("closesitepaneli");
    }
});
