// Bu dosya, hızlı erişim sayfasının davranışını yönetir, native taraftan en çok ziyaret edilen siteleri ister ve ekranda kutular halinde gösterir.
// Bu dosya, sadece bu dahili sayfa içinde çalışır, native taraf mesajları her zaman gerçek kaynak adresine (tarayici.local) bakarak doğruladığı için, bu sayfa güvenle ayrıcalıklı komutlar gönderebilir.

document.title = "Yeni Sekme"; // Sekme şeridinde bu sayfanın başlığı "Yeni Sekme" olarak görünsün diye sayfa başlığı burada ayarlanır

// Bu fonksiyon, verilen bir site adından, hızlı erişim kutusunda gösterilecek tek harfi (genellikle alan adının ilk harfi) döndürür
function ilkHarfiAl(metin) {
    return metin && metin.length > 0 ? metin.charAt(0) : "?"; // Metin boş değilse ilk karakteri, boşsa soru işareti döndürülür
}

// Native taraftan bir mesaj geldiğinde bu fonksiyon çalışır
window.chrome.webview.addEventListener("message", function (olay) {
    let veri; // Gelen mesajın JSON'dan çözülmüş halini tutacak değişken
    try {
        veri = JSON.parse(olay.data); // Native tarafın gönderdiği JSON metni, bir JavaScript nesnesine çevrilir
    } catch (hata) {
        return; // Eğer mesaj geçerli bir JSON değilse, güvenli bir şekilde yok sayılır
    }

    if (veri.type === "topsites") { // Eğer gelen mesaj, en çok ziyaret edilen siteler listesiyse
        const kapsayici = document.getElementById("hizli-erisim-kutulari"); // Kutuların ekleneceği kapsayıcı alınır
        kapsayici.innerHTML = ""; // Önceki içerik temizlenir

        if (veri.entries.length === 0) { // Eğer hiç geçmiş kaydı yoksa
            document.getElementById("bos-durum-mesaji").classList.remove("gizli"); // Boş durum mesajı gösterilir
            return; // Gösterilecek kutu olmadığı için fonksiyondan çıkılır
        }

        veri.entries.forEach(function (site) { // Her site için bir kutu oluşturulur
            const kutu = document.createElement("a"); // Tıklanabilir bir bağlantı öğesi oluşturulur
            kutu.className = "hizli-erisim-kutusu"; // Kutuya stil sınıfı atanır
            kutu.href = site.adres; // Kutuya tıklanınca gidilecek adres atanır, bu WebView2 içinde normal bir gezinme olarak çalışır

            const harfOgesi = document.createElement("div"); // Site ikonu gelmezse gösterilecek, ilk harfi taşıyan yedek daire öğesi
            harfOgesi.className = "hizli-erisim-harf"; // Daireye stil sınıfı atanır
            harfOgesi.textContent = ilkHarfiAl(site.baslik); // Sitenin ilk harfi yazılır

            let siteHostu = ""; // Google favicon servisine gönderilecek, adresten ayıklanan alan adı
            try { siteHostu = new URL(site.adres).hostname; } catch (hata) { siteHostu = ""; } // Adres bozuksa alan adı boş kalır, ikon istenmez

            if (siteHostu) { // Alan adı ayıklanabildiyse, harf yerine gerçek site ikonu denenir
                const ikonOgesi = document.createElement("img"); // Google'ın favicon servisinden gelecek ikon için resim öğesi
                ikonOgesi.className = "hizli-erisim-ikon"; // İkona stil sınıfı atanır
                ikonOgesi.src = "https://www.google.com/s2/favicons?domain=" + encodeURIComponent(siteHostu) + "&sz=64"; // Sayfa ilk ziyaret edildiğinde native taraf bu adresi zaten önbelleğe ısıtmıştır
                ikonOgesi.onerror = function () { ikonOgesi.replaceWith(harfOgesi); }; // İkon hiç bulunamazsa (örneğin site favicon sunmuyorsa), yedek harf dairesi onun yerine konur
                kutu.appendChild(ikonOgesi); // İkon, kutunun içine eklenir
            } else {
                kutu.appendChild(harfOgesi); // Alan adı yoksa doğrudan yedek harf dairesi eklenir
            }

            const isimOgesi = document.createElement("div"); // Site ismini gösterecek metin öğesi
            isimOgesi.className = "hizli-erisim-isim"; // Metne stil sınıfı atanır
            isimOgesi.textContent = site.baslik; // Site ismi (alan adı) yazılır

            kutu.appendChild(isimOgesi); // İsim metni, kutunun içine eklenir
            kapsayici.appendChild(kutu); // Hazırlanan kutu, kapsayıcıya eklenir
        });
    }
    else if (veri.type === "newtabbackground") { // Eğer gelen mesaj, kullanıcının seçtiği özel arka plan resmiyse
        if (veri.dataUrl) { // Kullanıcı bir resim seçmişse, satır içi stil, rastgele desen sınıfından her zaman önceliklidir
            document.body.style.backgroundImage = "url('" + veri.dataUrl + "')";
            document.body.style.backgroundSize = "cover";
            document.body.style.backgroundPosition = "center";
        } else { // Hiç resim seçilmemişse (veya seçilen resim artık diskte bulunamadıysa), satır içi stil temizlenir, rastgele desen sınıfı tekrar görünür olur
            document.body.style.backgroundImage = "";
        }
    }
    else if (veri.type === "theme") { // Eğer gelen mesaj, tema bilgisiyse
        const tema = veri.theme; // Tema alanları, iç içe geçmiş "theme" nesnesinden okunur
        document.documentElement.style.setProperty("--vurgu-rengi", tema.accent); // Vurgu rengi, hızlı erişim kutularındaki harf dairelerine anında yansır
        const koseDegerleri = { kesik: "4px", orta: "10px", yuvarlak: "18px" }; // Üç kademeli köşe yuvarlaklığı seçeneğinin gerçek piksel karşılıkları
        document.documentElement.style.setProperty("--radius", koseDegerleri[tema.radius] || "10px"); // Hızlı erişim kutularının köşe yuvarlaklığı güncellenir
        if (tema.mode === "acik") { document.documentElement.setAttribute("data-tema", "acik"); } // Açık tema özellikle seçildiyse eklenir
        else if (tema.mode === "yuksekkontrast") { document.documentElement.setAttribute("data-tema", "yuksekkontrast"); } // Yüksek kontrast tema özellikle seçildiyse eklenir
        else { document.documentElement.removeAttribute("data-tema"); } // Koyu veya otomatik modda özellik kaldırılır
    }
});

window.chrome.webview.postMessage("get_topsites"); // Sayfa yüklenir yüklenmez, en çok ziyaret edilen siteler native taraftan istenir
window.chrome.webview.postMessage("get_theme"); // Sayfa yüklenir yüklenmez, güncel tema da istenir
window.chrome.webview.postMessage("get_newtab_background"); // Sayfa yüklenir yüklenmez, kullanıcının seçtiği özel arka plan resmi de istenir
