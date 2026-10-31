// Bu dosya, karşılama (onboarding) ekranının davranışını yönetir: kullanıcının vurgu rengi ve arama motoru seçimini toplar, "Başla" veya "Atla" ile seçimi native tarafa gönderir.
// Bu sayfa, native tarafa her zaman tarayici.local kaynağından mesaj gönderdiği için (BrowserWindow.cpp bunu her mesajda doğrular), güvenle ayrıcalıklı "complete_onboarding" komutunu kullanabilir.

document.title = "Hoş Geldiniz"; // Sekme şeridinde bu sayfanın başlığı "Hoş Geldiniz" olarak görünsün diye sayfa başlığı burada ayarlanır

const renkSecenekleri = ["#3b82f6", "#22c55e", "#f59e0b", "#ef4444", "#a855f7", "#14b8a6"]; // Kullanıcının seçebileceği altı hazır vurgu rengi
const renkKapsayici = document.getElementById("renk-secenekleri"); // Renk noktalarının ekleneceği kapsayıcı
let seciliRenk = renkSecenekleri[0]; // Şu anda seçili olan vurgu rengi, varsayılan olarak ilk renk seçilidir

renkSecenekleri.forEach(function (renk, indeks) { // Her renk için bir nokta oluşturulur
    const nokta = document.createElement("div"); // Yeni bir renk noktası öğesi oluşturulur
    nokta.className = "renk-noktasi" + (indeks === 0 ? " secili" : ""); // İlk renk, başlangıçta seçili olarak işaretlenir
    nokta.style.backgroundColor = renk; // Noktanın arka plan rengi, bu seçeneğin rengine ayarlanır
    nokta.addEventListener("click", function () { // Bu noktaya tıklandığında
        seciliRenk = renk; // Seçili renk güncellenir
        document.querySelectorAll(".renk-noktasi").forEach(function (n) { n.classList.remove("secili"); }); // Önceki seçili işareti kaldırılır
        nokta.classList.add("secili"); // Bu nokta, yeni seçili olarak işaretlenir
        document.documentElement.style.setProperty("--vurgu-rengi", renk); // Seçilen renk, kartın kendi butonlarına da anında yansısın diye CSS değişkeni güncellenir
    });
    renkKapsayici.appendChild(nokta); // Hazırlanan nokta, kapsayıcıya eklenir
});

let seciliMotor = "google"; // Şu anda seçili olan arama motoru, varsayılan olarak Google seçilidir

document.querySelectorAll("#motor-secenekleri .segment").forEach(function (buton) { // Her arama motoru seçeneğine tıklama dinleyicisi eklenir
    buton.addEventListener("click", function () {
        seciliMotor = buton.dataset.motor; // Seçilen motor güncellenir
        document.querySelectorAll("#motor-secenekleri .segment").forEach(function (b) { b.classList.remove("secili"); }); // Önceki seçili işareti kaldırılır
        buton.classList.add("secili"); // Bu buton, yeni seçili olarak işaretlenir
    });
});

// Bu fonksiyon, kullanıcının seçimlerini native tarafa gönderip karşılama ekranını kapatır
function tamamla() {
    const veri = { accent: seciliRenk, searchEngine: seciliMotor }; // Gönderilecek veri hazırlanır
    window.chrome.webview.postMessage("complete_onboarding|" + JSON.stringify(veri)); // Komut, native tarafa gönderilir, native taraf bu sekmeyi otomatik olarak hızlı erişim sayfasına yönlendirecektir
}

document.getElementById("buton-varsayilan-tarayici").addEventListener("click", function () { // Windows'un "Varsayılan Uygulamalar" ayar sayfası açılır, karşılama ekranı kapanmaz
    window.chrome.webview.postMessage("open_default_apps_settings");
});

document.getElementById("buton-basla").addEventListener("click", tamamla); // "Başla" butonuna tıklanınca, seçimlerle birlikte tamamlanır
document.getElementById("buton-atla").addEventListener("click", tamamla); // "Atla" butonuna tıklanınca da, varsayılan (henüz değiştirilmemiş) seçimlerle birlikte tamamlanır, kullanıcı hiçbir şeyi değiştirmek zorunda kalmaz
