// Bu dosya, sertifika hatası panelinin davranışını yönetir: native taraftan gelen sertifika hatasını gösterir, "Geri Dön"/"Yine de Git" tıklamalarını native tarafa bildirir.
// Bu panel kendi ayrı WebView2 denetleyicisinde çalıştığı için, açılıp kapanma ve konumlanma işini native taraf (BrowserWindow.cpp) yönetir, bu dosya sadece panelin İÇİNDEKİ davranıştan sorumludur.

const sertifikaAciklama = document.getElementById("sertifika-aciklama"); // Hatanın türünü anlatan açıklama metninin gösterileceği öğe
const sertifikaAdres = document.getElementById("sertifika-adres"); // Hatanın oluştuğu adresin gösterileceği öğe
const sertifikaDetaylar = document.getElementById("sertifika-detaylar"); // Sertifikanın temel bilgilerinin gösterileceği öğe
const sertifikaGeriDonButonu = document.getElementById("sertifika-geri-don-butonu"); // "Geri Dön" butonu
const sertifikaYineDeGitButonu = document.getElementById("sertifika-yine-de-git-butonu"); // "Yine de Git" butonu

let suankiIstekId = null; // Şu anda panelde gösterilen sertifika hatasının kimlik numarası, henüz hiçbir hata gösterilmediyse boştur

// Bu fonksiyon, metin içindeki HTML'e özel karakterleri (<, >, & gibi) kaçışlayarak, sertifika bilgilerinin innerHTML ile güvenle gösterilebilmesini sağlar
// Sertifikanın "Konu" veya "Yayınlayan" alanı, kötü niyetli bir sunucu tarafından serbestçe seçilebildiği için, bu alanlar hiçbir zaman ham HTML olarak sayfaya yazılmaz
function htmlIcinKacisla(metin) {
    const geciciOge = document.createElement("div");
    geciciOge.textContent = metin;
    return geciciOge.innerHTML;
}

// Native taraftan bir mesaj geldiğinde (yeni bir sertifika hatası bildirildiğinde) bu fonksiyon çalışır
window.chrome.webview.addEventListener("message", function (olay) {
    let veri; // Gelen mesajın JSON'dan çözülmüş halini tutacak değişken
    try {
        veri = JSON.parse(olay.data); // Native tarafın gönderdiği JSON metni, bir JavaScript nesnesine çevrilir
    } catch (hata) {
        return; // Eğer mesaj geçerli bir JSON değilse, güvenli bir şekilde yok sayılır
    }

    if (veri.type !== "certificateerror") { return; } // Bu panel sadece kendi veri türüyle ilgilenir

    suankiIstekId = veri.requestId; // Hatanın kimlik numarası hatırlanır, yanıt gönderilirken kullanılır
    sertifikaAciklama.textContent = veri.description; // Hatanın türünü anlatan açıklama gösterilir
    sertifikaAdres.textContent = veri.uri; // Hatanın oluştuğu adres gösterilir
    sertifikaDetaylar.innerHTML =
        "<b>Konu:</b> " + htmlIcinKacisla(veri.subject) + "<br>" +
        "<b>Yayınlayan:</b> " + htmlIcinKacisla(veri.issuer) + "<br>" +
        "<b>Geçerlilik:</b> " + htmlIcinKacisla(veri.validFrom) + " – " + htmlIcinKacisla(veri.validTo);
});

// Bu fonksiyon, kullanıcının kararını native tarafa bildirir
function kararGonder(devamEt) {
    if (suankiIstekId === null) { // Gösterilen bir hata yoksa (örneğin panel henüz hiç veri almadan tıklandıysa)
        return; // Gönderilecek bir şey yoktur
    }
    window.chrome.webview.postMessage("certificate_response|" + suankiIstekId + "|" + (devamEt ? "proceed" : "cancel")); // Karar, istek kimliğiyle birlikte gönderilir
    suankiIstekId = null; // Yanıtlandığına göre, bu istek artık gösterilmiyor sayılır
}

sertifikaGeriDonButonu.addEventListener("click", function () { kararGonder(false); });
sertifikaYineDeGitButonu.addEventListener("click", function () { kararGonder(true); });

document.addEventListener("keydown", function (olay) {
    if (olay.key === "Escape") { // Kullanıcı Escape'e basarsa, bu da "Geri Dön" (güvenli taraf) kararı olarak sayılır
        kararGonder(false);
    }
});
