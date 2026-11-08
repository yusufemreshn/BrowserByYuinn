// Bu dosya, izin bildirim panelinin davranışını yönetir: native taraftan gelen izin isteğini gösterir, "İzin Ver"/"Engelle" tıklamalarını native tarafa bildirir.
// Bu panel kendi ayrı WebView2 denetleyicisinde çalıştığı için, açılıp kapanma ve konumlanma işini native taraf (BrowserWindow.cpp) yönetir, bu dosya sadece panelin İÇİNDEKİ davranıştan sorumludur.

const izinSimge = document.getElementById("izin-simge"); // İstenen izin türüne uygun ikonun çizileceği öğe
const izinSiteAdi = document.getElementById("izin-site-adi"); // İzni isteyen sitenin alan adının gösterileceği öğe
const izinAciklama = document.getElementById("izin-aciklama"); // İstenen izni açıklayan metnin gösterileceği öğe
const izinHatirlaKutusu = document.getElementById("izin-hatirla-kutusu"); // "Bu site için hatırla" onay kutusu
const izinIzinVerButonu = document.getElementById("izin-izin-ver-butonu"); // "İzin Ver" butonu
const izinEngelleButonu = document.getElementById("izin-engelle-butonu"); // "Engelle" butonu

let suankiIstekId = null; // Şu anda panelde gösterilen izin isteğinin kimlik numarası, henüz hiçbir istek gösterilmediyse boştur

// Her izin türü için, kullanıcıya gösterilecek doğal dilde bir açıklama
const izinTuruAciklamalari = {
    konum: "konumunuza erişmek istiyor",
    kamera: "kameranızı kullanmak istiyor",
    mikrofon: "mikrofonunuzu kullanmak istiyor",
    bildirim: "size bildirim göndermek istiyor",
    otomatikoynatma: "sesli içerikleri otomatik oynatmak istiyor"
};

// Her izin türü için, ikon rozetinde gösterilecek SVG çizimi; tek renk olduğu için "currentColor" kullanılır, rengi CSS belirler
const izinTuruSimgeleri = {
    konum: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M12 21s7-6.3 7-11.5A7 7 0 0 0 5 9.5C5 14.7 12 21 12 21Z"/><circle cx="12" cy="9.5" r="2.4"/></svg>',
    kamera: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="7" width="13" height="11" rx="2.2"/><path d="M16 10.3 21 7.5v9l-5-2.8"/></svg>',
    mikrofon: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><rect x="9" y="3" width="6" height="11" rx="3"/><path d="M5 11a7 7 0 0 0 14 0"/><path d="M12 18v3"/></svg>',
    bildirim: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M18 8a6 6 0 1 0-12 0c0 3.5-1 5.5-2 7h16c-1-1.5-2-3.5-2-7Z"/><path d="M10.5 21a1.5 1.5 0 0 0 3 0"/></svg>',
    otomatikoynatma: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="9"/><path d="M10 9.3v5.4L15 12l-5-2.7Z" fill="currentColor" stroke="none"/></svg>'
};

// Bu fonksiyon, bir sitenin tam kaynak adresinden (örneğin "https://example.com"), kullanıcıya gösterilecek sade alan adını çıkarır
function kaynaktanAlanAdiCikar(kaynak) {
    try {
        return new URL(kaynak).hostname; // Tarayıcının kendi URL ayrıştırıcısı kullanılır, adres bozuksa hata fırlatabilir
    } catch (hata) {
        return kaynak; // Ayrıştırılamazsa, kaynak olduğu gibi gösterilir
    }
}

// Native taraftan bir mesaj geldiğinde (yeni bir izin isteği bildirildiğinde) bu fonksiyon çalışır
window.chrome.webview.addEventListener("message", function (olay) {
    let veri; // Gelen mesajın JSON'dan çözülmüş halini tutacak değişken
    try {
        veri = JSON.parse(olay.data); // Native tarafın gönderdiği JSON metni, bir JavaScript nesnesine çevrilir
    } catch (hata) {
        return; // Eğer mesaj geçerli bir JSON değilse, güvenli bir şekilde yok sayılır
    }

    if (veri.type === "permissionrequest") { // Eğer gelen mesaj, gösterilecek bir izin isteğiyse
        suankiIstekId = veri.requestId; // İsteğin kimlik numarası hatırlanır, yanıt gönderilirken kullanılır
        izinSiteAdi.textContent = kaynaktanAlanAdiCikar(veri.origin); // Sitenin sade alan adı gösterilir
        izinAciklama.textContent = izinTuruAciklamalari[veri.kind] || "bir izin istiyor"; // İstenen izne uygun açıklama gösterilir, tanınmayan bir tür gelirse genel bir metin kullanılır
        izinSimge.innerHTML = izinTuruSimgeleri[veri.kind] || izinTuruSimgeleri.bildirim; // İzne uygun ikon gösterilir, tanınmayan bir tür gelirse bildirim ikonuna düşülür
        izinHatirlaKutusu.checked = true; // Her yeni istekte, "hatırla" kutusu varsayılan olarak işaretli başlar
    }
});

// Bu fonksiyon, kullanıcının kararını native tarafa bildirir
function kararGonder(izinVerildi) {
    if (suankiIstekId === null) { // Gösterilen bir istek yoksa (örneğin panel henüz hiç veri almadan tıklandıysa)
        return; // Gönderilecek bir şey yoktur
    }
    window.chrome.webview.postMessage("permission_response|" + suankiIstekId + "|" + (izinVerildi ? "allow" : "deny") + "|" + (izinHatirlaKutusu.checked ? "1" : "0")); // Karar, istek kimliği ve "hatırla" tercihiyle birlikte gönderilir
    suankiIstekId = null; // Yanıtlandığına göre, bu istek artık gösterilmiyor sayılır
}

izinIzinVerButonu.addEventListener("click", function () { kararGonder(true); });
izinEngelleButonu.addEventListener("click", function () { kararGonder(false); });

document.addEventListener("keydown", function (olay) {
    if (olay.key === "Escape") { // Kullanıcı Escape'e basarsa, bu da bir "Engelle" kararı olarak sayılır, panel sebepsiz yere açık kalmaz
        kararGonder(false);
    }
});
