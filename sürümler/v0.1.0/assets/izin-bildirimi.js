// Bu dosya, izin bildirim panelinin davranışını yönetir: native taraftan gelen izin isteğini gösterir, "İzin Ver"/"Engelle" tıklamalarını native tarafa bildirir.
// Bu panel kendi ayrı WebView2 denetleyicisinde çalıştığı için, açılıp kapanma ve konumlanma işini native taraf (BrowserWindow.cpp) yönetir, bu dosya sadece panelin İÇİNDEKİ davranıştan sorumludur.

const izinSiteAdi = document.getElementById("izin-site-adi"); // İzni isteyen sitenin alan adının gösterileceği öğe
const izinAciklama = document.getElementById("izin-aciklama"); // İstenen izni açıklayan metnin gösterileceği öğe
const izinHatirlaKutusu = document.getElementById("izin-hatirla-kutusu"); // "Bu site için hatırla" onay kutusu
const izinIzinVerButonu = document.getElementById("izin-izin-ver-butonu"); // "İzin Ver" butonu
const izinEngelleButonu = document.getElementById("izin-engelle-butonu"); // "Engelle" butonu

let suankiIstekId = null; // Şu anda panelde gösterilen izin isteğinin kimlik numarası, henüz hiçbir istek gösterilmediyse boştur

// Her izin türü için, kullanıcıya gösterilecek doğal dilde bir açıklama
const izinTuruAciklamalari = {
    konum: "konumunuza erişmek istiyor.",
    kamera: "kameranızı kullanmak istiyor.",
    mikrofon: "mikrofonunuzu kullanmak istiyor.",
    bildirim: "size bildirim göndermek istiyor.",
    otomatikoynatma: "sesli içerikleri otomatik oynatmak istiyor."
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
        izinAciklama.textContent = izinTuruAciklamalari[veri.kind] || "bir izin istiyor."; // İstenen izne uygun açıklama gösterilir, tanınmayan bir tür gelirse genel bir metin kullanılır
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
