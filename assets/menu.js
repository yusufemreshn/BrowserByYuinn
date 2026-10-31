// Bu dosya, hamburger menü panelinin davranışını yönetir: öğelere tıklanınca native tarafa komut gönderir, RAM/yakınlaştırma bilgisini gösterir.
// Bu panel artık kendi ayrı WebView2 denetleyicisinde çalıştığı için, açılıp kapanma ve konumlanma işini native taraf (BrowserWindow.cpp) yönetir, bu dosya sadece panelin İÇİNDEKİ davranıştan sorumludur.

const menuRamDeger = document.getElementById("menu-ram-deger"); // Menüdeki RAM göstergesinin değer kısmı
const menuYakinlastirYuzde = document.getElementById("menu-yakinlastir-yuzde"); // Menüdeki güncel yakınlaştırma yüzdesi
const menuYakinlastirAzalt = document.getElementById("menu-yakinlastir-azalt"); // Menüdeki uzaklaştırma (eksi) butonu
const menuYakinlastirArttir = document.getElementById("menu-yakinlastir-arttir"); // Menüdeki yakınlaştırma (artı) butonu

// Menüdeki normal öğeler (RAM göstergesi ve yakınlaştırma satırı hariç), tıklanınca kendi komutlarını native tarafa gönderir ve menüyü kapatır
document.querySelectorAll(".menu-ogesi").forEach(function (oge) {
    oge.addEventListener("click", function () {
        window.chrome.webview.postMessage(oge.dataset.komut); // Öğenin üzerindeki "data-komut" özelliğinde yazan komut, olduğu gibi native tarafa gönderilir
        window.chrome.webview.postMessage("closemenu"); // Bir komut seçildiğine göre, native taraftan menünün kapatılması istenir
    });
});

menuYakinlastirAzalt.addEventListener("click", function () {
    window.chrome.webview.postMessage("zoomout"); // Aktif sekme uzaklaştırılır, menü kapanmaz, kullanıcı art arda tıklayabilir
});
menuYakinlastirArttir.addEventListener("click", function () {
    window.chrome.webview.postMessage("zoomin"); // Aktif sekme yakınlaştırılır, menü kapanmaz, kullanıcı art arda tıklayabilir
});

document.addEventListener("keydown", function (olay) {
    if (olay.key === "Escape") { // Panel açıkken Escape tuşuna basıldıysa
        window.chrome.webview.postMessage("closemenu"); // Native taraftan menünün kapatılması istenir
    }
});

// Native taraftan bir mesaj geldiğinde bu fonksiyon çalışır (menü her açıldığında güncel RAM ve yakınlaştırma bilgisi gönderilir)
window.chrome.webview.addEventListener("message", function (olay) {
    let veri; // Gelen mesajın JSON'dan çözülmüş halini tutacak değişken
    try {
        veri = JSON.parse(olay.data); // Native tarafın gönderdiği JSON metni, bir JavaScript nesnesine çevrilir
    } catch (hata) {
        return; // Eğer mesaj geçerli bir JSON değilse, güvenli bir şekilde yok sayılır
    }

    if (veri.type === "ramstatus") { // Eğer gelen mesaj güncel RAM kullanımıysa
        menuRamDeger.textContent = veri.ramMB + " MB"; // Menüdeki RAM göstergesi güncellenir
    }
    else if (veri.type === "zoom") { // Eğer gelen mesaj güncel yakınlaştırma yüzdesiyse
        menuYakinlastirYuzde.textContent = veri.zoomPercent + "%"; // Menüdeki yakınlaştırma yüzdesi güncellenir
    }
});
