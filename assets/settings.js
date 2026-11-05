// Bu dosya, dahili Ayarlar sekmesinin davranışını yönetir, yani kategoriler arasında geçişi, her panelin native taraftan veri isteyip ekranda göstermesini, ve kullanıcının yaptığı değişiklikleri native tarafa göndermesini sağlar.
// Bu dosya, sadece Ayarlar sekmesi içinde çalışır, normal bir internet sitesinde asla yüklenmez, bu yüzden native taraf bu sayfadan gelen mesajlara ayrıcalıklı komutlar olarak güvenir.
// Native taraf ile konuşma, chrome.js'dekiyle aynı yönteme dayanır, window.chrome.webview.postMessage ile komut gönderilir, window.chrome.webview'in "message" olayıyla JSON yanıtlar alınır.

document.title = "Ayarlar"; // Sekme şeridinde bu sayfanın başlığı "Ayarlar" olarak görünsün diye sayfa başlığı burada ayarlanır, çünkü bu sayfanın kendi <title> etiketi sekme şeridine otomatik yansımaz

const kategoriButonlari = document.querySelectorAll(".kategori"); // Sol menüdeki tüm kategori butonları seçilir
const panelOgeleri = document.querySelectorAll(".panel"); // Sağdaki tüm panel bölümleri seçilir

let hakkindaYenilemeZamanlayicisi = null; // Hakkında paneli açıkken RAM ve çalışma süresini periyodik olarak tazeleyen zamanlayıcının kimliği, panelden çıkılınca durdurulması için saklanır

const aramaKutusu = document.getElementById("genel-arama"); // Üstteki tek, paylaşılan arama kutusu, aktif kategoriye göre doğru listeyi süzer
const aramaTemizleButonu = document.getElementById("arama-temizle"); // Arama kutusunun içindeki küçük temizleme düğmesi
const durumMetni = document.getElementById("durum-metni"); // Alt durum çubuğundaki, kaç öğenin göründüğünü yazan metin

const kategoriAramaYeriTutucusu = { // Her kategoride arama kutusunun içinde görünecek örnek metin, kullanıcıya nerede arama yaptığını hatırlatır
    genel: "Ayarlarda ara…", gorunum: "Ayarlarda ara…", gizlilik: "Ayarlarda ara…",
    sifreler: "Şifrelerde ara…", doldurma: "Ayarlarda ara…", gecmis: "Geçmişte ara…", yerimleri: "Yer imlerinde ara…", okumalistesi: "Okuma listesinde ara…", indirmeler: "Ayarlarda ara…",
    hatagunlugu: "Hata günlüğünde ara…", eklentiler: "Ayarlarda ara…", kisayollar: "Ayarlarda ara…", hakkinda: "Ayarlarda ara…"
};

// Bu fonksiyon, şu anda aktif olan panelin içindeki satırları, arama kutusundaki metne göre gösterir veya gizler
function aramaFiltresiUygula() {
    const sorgu = aramaKutusu.value.trim().toLowerCase(); // Arama kutusundaki metin, küçük harfe çevrilerek okunur
    const aktifPanel = document.querySelector(".panel.aktif"); // Şu anda görünen panel bulunur
    if (!aktifPanel) { return; } // Hiçbir panel aktif değilse yapılacak bir şey yoktur

    const satirlar = aktifPanel.querySelectorAll(".ayar-satiri, .liste-satiri"); // Bu panelin içindeki tüm ayar ve liste satırları bulunur
    let gorunenSayisi = 0; // Sorguya uyup görünür kalan satır sayısı
    satirlar.forEach(function (satir) { // Her satır sırayla gezilir
        const eslesiyor = !sorgu || satir.textContent.toLowerCase().includes(sorgu); // Sorgu boşsa ya da satırın metni sorguyu içeriyorsa eşleşme sayılır
        satir.style.display = eslesiyor ? "" : "none"; // Eşleşmeyen satırlar gizlenir
        if (eslesiyor) { gorunenSayisi++; } // Görünen satır sayısı güncellenir
    });

    durumCubugunuGuncelle(gorunenSayisi, sorgu); // Alt durum çubuğu, güncel görünen sayı ve sorguya göre yenilenir
}

// Bu fonksiyon, alt durum çubuğundaki metni, verilen görünen öğe sayısına ve arama sorgusuna göre günceller
function durumCubugunuGuncelle(gorunenSayisi, sorgu) {
    const aktifKategoriButonu = document.querySelector(".kategori.aktif"); // Aktif kategori butonu bulunur
    const kategoriAdi = aktifKategoriButonu ? aktifKategoriButonu.querySelector(".kategori-ad").textContent : ""; // Butonun görünen ismi okunur
    if (sorgu) { // Eğer kullanıcı bir şey aradıysa
        durumMetni.textContent = gorunenSayisi + " eşleşme · \"" + sorgu + "\""; // Eşleşme sayısı ve sorgu gösterilir
    } else { // Arama kutusu boşsa
        durumMetni.textContent = gorunenSayisi + " öğe · " + kategoriAdi; // Görünen öğe sayısı ve kategori adı gösterilir
    }
    aramaTemizleButonu.classList.toggle("gorunur", sorgu.length > 0); // Temizleme düğmesi, sadece arama kutusunda metin varken gösterilir
}

// Bu fonksiyon, arama kutusuna her yazıldığında, aktif kategoriye göre doğru süzme mantığını çalıştırır
function aramayiUygula() {
    const aktifKategoriButonu = document.querySelector(".kategori.aktif"); // Aktif kategori butonu bulunur
    const aktifKategori = aktifKategoriButonu ? aktifKategoriButonu.dataset.kategori : ""; // Aktif kategorinin adı okunur
    if (aktifKategori === "gecmis") { gecmisListesiniCiz(); } // Geçmiş panelindeyken, native listeden süzülerek yeniden çizilir
    else if (aktifKategori === "hatagunlugu") { hataListesiniCiz(); } // Hata günlüğü panelindeyken, aynı şekilde native listeden süzülür
    else if (aktifKategori === "yerimleri") { yerimleriListesiniCiz(); } // Yer imleri panelindeyken, aynı şekilde native listeden süzülür
    else if (aktifKategori === "okumalistesi") { okumaListesiniCiz(); } // Okuma listesi panelindeyken, aynı şekilde native listeden süzülür
    else { aramaFiltresiUygula(); } // Diğer panellerde, DOM üzerindeki satırlar doğrudan gösterilip gizlenir
}

aramaKutusu.addEventListener("input", aramayiUygula); // Arama kutusuna her yazıldığında doğru süzme fonksiyonu çalışır

aramaTemizleButonu.addEventListener("click", function () { // Temizleme düğmesine tıklandığında
    aramaKutusu.value = ""; // Arama kutusu boşaltılır
    aramaKutusu.focus(); // Odak tekrar arama kutusuna verilir
    aramayiUygula(); // Süzme, boş sorguyla yeniden uygulanır
});

document.addEventListener("keydown", function (olay) { // Sayfanın herhangi bir yerinde tuş basımlarını dinler
    if ((olay.ctrlKey || olay.metaKey) && olay.key.toLowerCase() === "k") { // Ctrl+K (veya Mac'te Cmd+K) basıldıysa
        olay.preventDefault(); // Tarayıcının kendi varsayılan kısayol davranışı engellenir
        aramaKutusu.focus(); // Arama kutusuna odaklanılır
        aramaKutusu.select(); // İçindeki metin seçili hale getirilir, böylece yazmaya başlayınca üzerine yazılır
    } else if (olay.key === "Escape" && document.activeElement === aramaKutusu && aramaKutusu.value) { // Esc'ye basıldığında, odak arama kutusundaysa ve içinde metin varsa
        aramaKutusu.value = ""; // Arama kutusu boşaltılır
        aramayiUygula(); // Süzme, boş sorguyla yeniden uygulanır
    }
});

// Bu fonksiyon, kullanıcı sol menüden bir kategoriye tıkladığında, o kategoriye ait paneli gösterir, diğerlerini gizler
function kategoriyeGec(kategoriAdi) {
    kategoriButonlari.forEach(function (buton) { // Tüm kategori butonları gezilir
        buton.classList.toggle("aktif", buton.dataset.kategori === kategoriAdi); // Sadece tıklanan kategoriye ait buton "aktif" sınıfını alır
    });
    panelOgeleri.forEach(function (panel) { // Tüm paneller gezilir
        panel.classList.toggle("aktif", panel.id === "panel-" + kategoriAdi); // Sadece seçilen kategoriye ait panel görünür yapılır
    });

    // Kategoriye geçildiğinde, o kategorinin güncel verisi native taraftan istenir, böylece liste her zaman güncel kalır
    if (kategoriAdi === "gecmis") { window.chrome.webview.postMessage("get_history"); }
    else if (kategoriAdi === "sifreler") { window.chrome.webview.postMessage("get_passwords"); }
    else if (kategoriAdi === "doldurma") { window.chrome.webview.postMessage("get_autofillprofile"); window.chrome.webview.postMessage("get_cards"); }
    else if (kategoriAdi === "indirmeler") { window.chrome.webview.postMessage("get_downloads"); }
    else if (kategoriAdi === "eklentiler") { window.chrome.webview.postMessage("get_extensions"); window.chrome.webview.postMessage("get_userscripts"); }
    else if (kategoriAdi === "hatagunlugu") { window.chrome.webview.postMessage("get_errorlog"); }
    else if (kategoriAdi === "hakkinda") { window.chrome.webview.postMessage("get_about"); }
    else if (kategoriAdi === "gizlilik") { window.chrome.webview.postMessage("get_permissions"); }
    else if (kategoriAdi === "yerimleri") { window.chrome.webview.postMessage("get_bookmarks"); }
    else if (kategoriAdi === "okumalistesi") { window.chrome.webview.postMessage("get_readinglist"); }

    if (hakkindaYenilemeZamanlayicisi !== null) { // Başka bir kategoriye geçildiğinde, eğer Hakkında paneli için çalışan bir zamanlayıcı varsa
        clearInterval(hakkindaYenilemeZamanlayicisi); // Bu zamanlayıcı durdurulur, böylece görünmeyen bir panel için gereksiz yere arka planda veri istenmeye devam edilmez
        hakkindaYenilemeZamanlayicisi = null; // Zamanlayıcı kimliği sıfırlanır
    }

    if (kategoriAdi === "hakkinda") { // Eğer kullanıcı tam olarak Hakkında paneline geçtiyse
        hakkindaYenilemeZamanlayicisi = setInterval(function () { // İki saniyede bir tekrar eden yeni bir zamanlayıcı başlatılır
            window.chrome.webview.postMessage("get_about"); // Güncel RAM ve çalışma süresi bilgisi tekrar istenir
        }, 2000); // İki saniyelik aralık, hem güncel kalmasını sağlar hem de gereğinden sık istek göndermez
    }

    aramaKutusu.value = ""; // Kategori değiştiğinde, önceki kategoriye ait arama sorgusu temizlenir
    aramaKutusu.placeholder = kategoriAramaYeriTutucusu[kategoriAdi] || "Ayarlarda ara…"; // Arama kutusunun örnek metni, yeni kategoriye göre güncellenir
    aramaFiltresiUygula(); // Yeni panelin tüm satırları, boş sorguyla görünür hale getirilir ve durum çubuğu güncellenir
}

kategoriButonlari.forEach(function (buton) { // Her kategori butonuna tıklama dinleyicisi eklenir
    buton.addEventListener("click", function () {
        kategoriyeGec(buton.dataset.kategori); // Tıklanan kategoriye geçilir
    });
});

// Genel panel, arama motoru ve ana sayfa ayarlarını native tarafa kaydeder
document.getElementById("buton-genel-kaydet").addEventListener("click", function () {
    const secilenMotor = document.getElementById("secim-arama-motoru").value; // Kullanıcının seçtiği arama motoru okunur
    const yeniAnaSayfa = document.getElementById("girdi-ana-sayfa").value; // Kullanıcının yazdığı ana sayfa adresi okunur
    window.chrome.webview.postMessage("set_searchengine|" + secilenMotor); // Arama motoru değişikliği native tarafa gönderilir
    window.chrome.webview.postMessage("set_homepage|" + yeniAnaSayfa); // Ana sayfa değişikliği native tarafa gönderilir
});

// Görünüm panelindeki vurgu renk seçenekleri, sabit bir renk listesinden oluşturulur, bunun yanında bir de serbest (özel) renk seçici bulunur
const renkSecenekleri = ["#3b82f6", "#22c55e", "#f59e0b", "#ef4444", "#a855f7", "#14b8a6"]; // Kullanıcının tek tıkla seçebileceği altı hazır vurgu rengi
const renkKapsayici = document.getElementById("renk-secenekleri"); // Renk noktalarının ekleneceği kapsayıcı
const ozelRenkGirdisi = document.getElementById("ozel-renk-girdisi"); // Kullanıcının tamamen serbest bir renk seçebileceği native renk seçici girdisi
let seciliVurguRengi = renkSecenekleri[0]; // Şu anda seçili olan vurgu rengi, native taraftan gelen ayarla güncellenecektir

renkSecenekleri.forEach(function (renk) { // Her hazır renk için bir nokta oluşturulur
    const nokta = document.createElement("div"); // Yeni bir renk noktası öğesi oluşturulur
    nokta.className = "renk-karesi"; // Renk noktasına stil sınıfı atanır
    nokta.style.backgroundColor = renk; // Noktanın arka plan rengi, bu seçeneğin rengine ayarlanır
    nokta.dataset.renk = renk; // Noktanın hangi renge karşılık geldiği, bir veri özelliğinde saklanır
    nokta.addEventListener("click", function () { // Bu noktaya tıklandığında
        seciliVurguRengi = renk; // Seçili renk güncellenir
        temaKaydet(); // Yeni tema, native tarafa kaydedilir
    });
    renkKapsayici.insertBefore(nokta, ozelRenkGirdisi); // Hazırlanan nokta, özel renk seçicinin hemen önüne eklenir, böylece özel seçici her zaman en sonda durur
});

ozelRenkGirdisi.addEventListener("input", function () { // Kullanıcı, native renk seçiciden bir renk belirlediğinde (sürüklerken anlık olarak tetiklenir)
    seciliVurguRengi = ozelRenkGirdisi.value; // Seçili renk, seçilen özel renge güncellenir
    temaKaydet(); // Yeni tema, native tarafa kaydedilir
});

let seciliYogunluk = "normal"; // Şu anda seçili olan sekme şeridi yoğunluğu
let seciliYaziBoyutu = "orta"; // Şu anda seçili olan yazı boyutu
let seciliTemaModu = "otomatik"; // Şu anda seçili olan açık/koyu/otomatik tema modu
let seciliKose = "orta"; // Şu anda seçili olan köşe yuvarlaklığı
let seciliYerImiCubugu = true; // Şu anda seçili olan yer imi çubuğu görünürlüğü

const anahtarYerImiCubugu = document.getElementById("anahtar-yerimi-cubugu"); // Yer imi çubuğu açık/kapalı anahtarının HTML öğesi
anahtarYerImiCubugu.addEventListener("change", function () {
    seciliYerImiCubugu = anahtarYerImiCubugu.checked; // Seçili durum, anahtarın yeni durumuna güncellenir
    temaKaydet(); // Yeni tercih, native tarafa kaydedilir
});

document.querySelectorAll("#yogunluk-secenekleri .segment").forEach(function (buton) { // Yoğunluk seçim butonlarının her birine tıklama dinleyicisi eklenir
    buton.addEventListener("click", function () {
        seciliYogunluk = buton.dataset.yogunluk; // Seçilen yoğunluk güncellenir
        temaKaydet(); // Yeni tema, native tarafa kaydedilir
    });
});

document.querySelectorAll("#yazi-boyutu-secenekleri .segment").forEach(function (buton) { // Yazı boyutu seçim butonlarının her birine tıklama dinleyicisi eklenir
    buton.addEventListener("click", function () {
        seciliYaziBoyutu = buton.dataset.boyut; // Seçilen yazı boyutu güncellenir
        temaKaydet(); // Yeni tema, native tarafa kaydedilir
    });
});

document.querySelectorAll("#tema-modu-secenekleri .segment").forEach(function (buton) { // Tema modu seçim butonlarının her birine tıklama dinleyicisi eklenir
    buton.addEventListener("click", function () {
        seciliTemaModu = buton.dataset.mod; // Seçilen tema modu güncellenir
        temaKaydet(); // Yeni tema, native tarafa kaydedilir
    });
});

document.querySelectorAll("#kose-secenekleri .segment").forEach(function (buton) { // Köşe yuvarlaklığı seçim butonlarının her birine tıklama dinleyicisi eklenir
    buton.addEventListener("click", function () {
        seciliKose = buton.dataset.kose; // Seçilen köşe yuvarlaklığı güncellenir
        temaKaydet(); // Yeni tema, native tarafa kaydedilir
    });
});

document.querySelectorAll(".preset-karti").forEach(function (kart) { // Her hazır tema kartına tıklama dinleyicisi eklenir
    kart.addEventListener("click", function () {
        seciliVurguRengi = kart.dataset.renk; // Kartın rengi, seçili renk yapılır
        seciliKose = kart.dataset.radius; // Kartın köşe yuvarlaklığı, seçili köşe yapılır
        seciliTemaModu = kart.dataset.mod; // Kartın tema modu, seçili mod yapılır
        temaKaydet(); // Hazır temanın tüm alanları, tek seferde native tarafa kaydedilir
    });
});

// Bu fonksiyon, şu anda seçili olan vurgu rengi, yoğunluk, yazı boyutu, köşe yuvarlaklığı ve tema modunu, tek bir JSON nesnesi olarak native tarafa gönderir
function temaKaydet() {
    const veri = { accent: seciliVurguRengi, density: seciliYogunluk, fontSize: seciliYaziBoyutu, radius: seciliKose, mode: seciliTemaModu, showBookmarkBar: (seciliYerImiCubugu ? "evet" : "hayir") }; // Gönderilecek veri, bir JavaScript nesnesi olarak hazırlanır
    window.chrome.webview.postMessage("set_theme|" + JSON.stringify(veri)); // Komut önekiyle birlikte, veri JSON metnine çevrilip gönderilir
}

// Bu fonksiyon, görünüm panelindeki seçili renk noktasını, özel renk seçiciyi ve segment butonlarını, verilen tema bilgisine göre günceller
function gorunumSecimleriniGuncelle(tema) {
    const hazirRenkteMi = renkSecenekleri.includes(tema.accent); // Güncel vurgu renginin, altı hazır renkten biri olup olmadığı kontrol edilir
    document.querySelectorAll(".renk-karesi").forEach(function (nokta) { // Her hazır renk noktası gezilir
        nokta.classList.toggle("secili", nokta.dataset.renk === tema.accent); // Sadece güncel vurgu rengine karşılık gelen nokta "seçili" olarak işaretlenir
    });
    if (!hazirRenkteMi) { // Eğer güncel renk, hazır renklerden biri değilse, kullanıcı özel bir renk seçmiş demektir
        ozelRenkGirdisi.value = tema.accent; // Özel renk seçicinin kendisi, bu rengi gösterecek şekilde güncellenir
    }

    document.querySelectorAll("#yogunluk-secenekleri .segment").forEach(function (buton) { // Her yoğunluk butonu gezilir
        buton.classList.toggle("secili", buton.dataset.yogunluk === tema.density); // Sadece güncel yoğunluğa karşılık gelen buton işaretlenir
    });
    document.querySelectorAll("#yazi-boyutu-secenekleri .segment").forEach(function (buton) { // Her yazı boyutu butonu gezilir
        buton.classList.toggle("secili", buton.dataset.boyut === tema.fontSize); // Sadece güncel yazı boyutuna karşılık gelen buton işaretlenir
    });
    document.querySelectorAll("#tema-modu-secenekleri .segment").forEach(function (buton) { // Her tema modu butonu gezilir
        buton.classList.toggle("secili", buton.dataset.mod === tema.mode); // Sadece güncel tema moduna karşılık gelen buton işaretlenir
    });
    document.querySelectorAll("#kose-secenekleri .segment").forEach(function (buton) { // Her köşe yuvarlaklığı butonu gezilir
        buton.classList.toggle("secili", buton.dataset.kose === tema.radius); // Sadece güncel köşe yuvarlaklığına karşılık gelen buton işaretlenir
    });

    anahtarYerImiCubugu.checked = (tema.showBookmarkBar !== false); // Yer imi çubuğu anahtarı, güncel tercihe göre ayarlanır

    seciliVurguRengi = tema.accent; seciliYogunluk = tema.density; seciliYaziBoyutu = tema.fontSize; seciliTemaModu = tema.mode; seciliKose = tema.radius; seciliYerImiCubugu = (tema.showBookmarkBar !== false); // Yerel değişkenler de güncel ayarla eşitlenir, böylece bir sonraki kullanıcı etkileşiminde eski değerlerin üzerine yazılmaz
}

// Gizlilik panelindeki zaman aralığı segmentleri, tek seferde sadece biri seçili olacak şekilde çalışır
let seciliVeriZamanKodu = "tum"; // Şu anda seçili olan zaman aralığı kodu, varsayılan olarak "tüm zamanlar"
document.querySelectorAll("#veri-zaman-secenekleri .segment").forEach(function (buton) { // Her zaman aralığı butonuna tıklama dinleyicisi eklenir
    buton.addEventListener("click", function () {
        seciliVeriZamanKodu = buton.dataset.zaman; // Seçilen zaman kodu güncellenir
        document.querySelectorAll("#veri-zaman-secenekleri .segment").forEach(function (b) { b.classList.toggle("secili", b === buton); }); // Sadece tıklanan buton "seçili" görünür
    });
});

// Güvenli gezinme uyarısı anahtarı değiştiğinde native tarafa kaydedilir
document.getElementById("anahtar-guvenli-gezinme").addEventListener("change", function (olay) {
    window.chrome.webview.postMessage("set_safebrowsing|" + (olay.target.checked ? "evet" : "hayir"));
});

// Reklam engelleme anahtarı değiştiğinde native tarafa kaydedilir
document.getElementById("anahtar-reklam-engelleme").addEventListener("change", function (olay) {
    window.chrome.webview.postMessage("set_adblock|" + (olay.target.checked ? "evet" : "hayir"));
});

// İzleyici engelleme seviyesi seçenekleri, tıklanınca hem görsel olarak işaretlenir hem native tarafa kaydedilir
document.querySelectorAll("#izleyici-seviye-secenekleri .segment").forEach(function (buton) {
    buton.addEventListener("click", function () {
        document.querySelectorAll("#izleyici-seviye-secenekleri .segment").forEach(function (b) { b.classList.toggle("secili", b === buton); });
        window.chrome.webview.postMessage("set_trackerlevel|" + buton.dataset.seviye);
    });
});

// Gizlilik panelindeki "Temizle" butonu, seçili zaman aralığı ve işaretli kategorilerle ayrıntılı veri temizleme komutu gönderir
document.getElementById("buton-veri-temizle").addEventListener("click", function () {
    const seciliKategoriler = []; // İşaretli kategori adlarının toplanacağı liste
    document.querySelectorAll("#veri-kategori-secenekleri input[type=\"checkbox\"]").forEach(function (kutu) { // Her kategori kutusu gezilir
        if (kutu.checked) { seciliKategoriler.push(kutu.dataset.kategori); } // İşaretli olanlar listeye eklenir
    });

    if (seciliKategoriler.length === 0) { // Hiçbir kategori işaretli değilse
        alert("Lütfen en az bir veri kategorisi seçin."); // Kullanıcı uyarılır
        return; // İşlem durdurulur
    }

    if (confirm("Seçtiğiniz veriler kalıcı olarak silinecek, devam edilsin mi?")) { // Kullanıcıya geri dönülemez bir işlem olduğu hatırlatılır
        window.chrome.webview.postMessage("clear_data|" + seciliVeriZamanKodu + "|" + seciliKategoriler.join(",")); // Onaylandıysa, ayrıntılı temizleme komutu native tarafa gönderilir
    }
});

// Geçmiş listesini, native taraftan gelen kayıtlarla ekrana çizen fonksiyon, mevcut arama kutusu metnine göre de filtreleme yapar
let sonGecmisListesi = []; // En son native taraftan gelen tam geçmiş listesi, arama kutusuna yazıldıkça tekrar native tarafa sormadan filtrelemek için saklanır

function gecmisListesiniCiz() {
    const aramaMetni = aramaKutusu.value.toLowerCase(); // Üstteki paylaşılan arama kutusuna yazılan metin, küçük harfe çevrilerek okunur
    const kapsayici = document.getElementById("gecmis-listesi"); // Listenin çizileceği kapsayıcı
    kapsayici.innerHTML = ""; // Önceki içerik temizlenir

    const filtrelenmis = sonGecmisListesi.filter(function (kayit) { // Tam listeden, arama metnine uyan kayıtlar süzülür
        return kayit.baslik.toLowerCase().includes(aramaMetni) || kayit.adres.toLowerCase().includes(aramaMetni); // Başlıkta veya adreste arama metni geçen kayıtlar tutulur
    });

    // Kaydın zaman damgasına (UTC ISO metni) göre, bugün/dün/geçen hafta/daha eski grup adını döndüren yardımcı fonksiyon
    function gecmisGrupAdi(zamanIso) {
        const kayitTarihi = new Date(zamanIso);
        const simdi = new Date();
        const gunBaslangici = function (tarih) { return new Date(tarih.getFullYear(), tarih.getMonth(), tarih.getDate()).getTime(); };
        const gunFarki = Math.floor((gunBaslangici(simdi) - gunBaslangici(kayitTarihi)) / 86400000);
        if (gunFarki <= 0) { return "Bugün"; }
        if (gunFarki === 1) { return "Dün"; }
        if (gunFarki <= 7) { return "Geçen Hafta"; }
        return "Daha Eski";
    }

    let sonGrupAdi = null; // Art arda aynı gruba ait kayıtlarda tekrar başlık çizilmemesi için son çizilen grup adı tutulur
    for (let i = filtrelenmis.length - 1; i >= 0; i--) { // Liste, en yeni kayıt en üstte olacak şekilde ters sırada gezilir
        const kayit = filtrelenmis[i]; // Gösterilecek kayıt

        const grupAdi = gecmisGrupAdi(kayit.zaman);
        if (grupAdi !== sonGrupAdi) { // Yeni bir gruba geçildiyse, bir grup başlığı çizilir
            sonGrupAdi = grupAdi;
            const grupBasligi = document.createElement("div");
            grupBasligi.className = "gecmis-grup-basligi";
            grupBasligi.textContent = grupAdi;
            kapsayici.appendChild(grupBasligi);
        }

        const satir = document.createElement("div"); // Bu kayıt için yeni bir satır öğesi oluşturulur
        satir.className = "liste-satiri"; // Satıra stil sınıfı atanır

        const anaMetin = document.createElement("div"); // Başlık ve adresi gösterecek metin öğesi
        anaMetin.className = "ana-metin"; // Stil sınıfı atanır
        anaMetin.innerHTML = kayit.baslik + "<div class=\"ikincil-metin\">" + kayit.adres + "</div>"; // Başlık ve adres, metnin içine yazılır

        const silButonu = document.createElement("button"); // Bu kayda özel "Sil" butonu oluşturulur
        silButonu.textContent = "Sil"; // Buton metni ayarlanır
        silButonu.addEventListener("click", function () {
            window.chrome.webview.postMessage("history_delete_entry|" + JSON.stringify({ adres: kayit.adres, zaman: kayit.zaman })); // Native tarafa, bu kaydın silinmesi komutu gönderilir
        });

        satir.appendChild(anaMetin); // Ana metin, satıra eklenir
        satir.appendChild(silButonu); // Sil butonu, satıra eklenir
        kapsayici.appendChild(satir); // Hazırlanan satır, listeye eklenir
    }

    aramaFiltresiUygula(); // Liste yeniden çizildiği için, durum çubuğu ve görünürlük bu yeni satırlara göre tazelenir
}

// Hata günlüğü listesini, native taraftan gelen kayıtlarla ekrana çizen fonksiyon
let sonHataListesi = []; // En son native taraftan gelen tam hata listesi

function hataListesiniCiz() {
    const aramaMetni = aramaKutusu.value.toLowerCase(); // Üstteki paylaşılan arama kutusuna yazılan metin okunur
    const kapsayici = document.getElementById("hata-listesi"); // Listenin çizileceği kapsayıcı
    kapsayici.innerHTML = ""; // Önceki içerik temizlenir

    const filtrelenmis = sonHataListesi.filter(function (kayit) { // Tam listeden, arama metnine uyan kayıtlar süzülür
        return kayit.kod.toLowerCase().includes(aramaMetni) || kayit.aciklama.toLowerCase().includes(aramaMetni); // Kodda veya açıklamada arama metni geçen kayıtlar tutulur
    });

    filtrelenmis.forEach(function (kayit) { // Her hata kaydı için
        const satir = document.createElement("div"); // Yeni bir satır öğesi oluşturulur
        satir.className = "liste-satiri"; // Satıra stil sınıfı atanır
        satir.innerHTML = "<div class=\"ana-metin\"><span class=\"hata-kodu\">" + kayit.kod + "</span> — " + kayit.aciklama + "<div class=\"ikincil-metin\">" + kayit.tarih + " " + kayit.saat + " · " + kayit.mesaj + "</div></div>"; // Kod, açıklama, tarih/saat ve ek mesaj satırın içine yazılır
        kapsayici.appendChild(satir); // Hazırlanan satır, listeye eklenir
    });

    aramaFiltresiUygula(); // Liste yeniden çizildiği için, durum çubuğu ve görünürlük bu yeni satırlara göre tazelenir
}

// Şifre kasası kilitlenme süresi seçimi, değiştirildiği anda native tarafa kaydedilir
document.getElementById("secim-kilitlenme-suresi").addEventListener("change", function (olay) {
    window.chrome.webview.postMessage("set_lockminutes|" + olay.target.value);
});

// Çıkışta otomatik temizleme anahtarı veya kategorileri değiştiğinde, native tarafa birlikte kaydedilir
function cikistaTemizleAyariniGonder() {
    const acikMi = document.getElementById("anahtar-cikista-temizle").checked ? "evet" : "hayir";
    const seciliKategoriler = Array.from(document.querySelectorAll("#cikista-temizle-kategoriler input[data-cikis-kategori]:checked")).map(function (kutu) { return kutu.dataset.cikisKategori; });
    window.chrome.webview.postMessage("set_exitclean|" + acikMi + "|" + seciliKategoriler.join(","));
}
document.getElementById("anahtar-cikista-temizle").addEventListener("change", cikistaTemizleAyariniGonder);
document.querySelectorAll("#cikista-temizle-kategoriler input[data-cikis-kategori]").forEach(function (kutu) {
    kutu.addEventListener("change", cikistaTemizleAyariniGonder);
});

// Hariç tutulan site listesini, native taraftan gelen kayıtlarla ekrana çizen fonksiyon
let sonHaricTutulanSiteler = []; // En son native taraftan gelen tam liste

function haricTutulanSiteleriCiz() {
    const kapsayici = document.getElementById("haric-tutulan-siteler-listesi");
    kapsayici.innerHTML = "";
    sonHaricTutulanSiteler.forEach(function (site) {
        const satir = document.createElement("div");
        satir.className = "liste-satiri";

        const anaMetin = document.createElement("div");
        anaMetin.className = "ana-metin";
        anaMetin.textContent = site;

        const silButonu = document.createElement("button");
        silButonu.textContent = "Kaldır";
        silButonu.addEventListener("click", function () {
            sonHaricTutulanSiteler = sonHaricTutulanSiteler.filter(function (s) { return s !== site; });
            window.chrome.webview.postMessage("set_exemptsites|" + sonHaricTutulanSiteler.join("|"));
        });

        satir.appendChild(anaMetin);
        satir.appendChild(silButonu);
        kapsayici.appendChild(satir);
    });
}

document.getElementById("buton-haric-site-ekle").addEventListener("click", function () {
    const girdi = document.getElementById("girdi-haric-site");
    const deger = girdi.value.trim();
    if (!deger) { return; }
    if (sonHaricTutulanSiteler.indexOf(deger) === -1) { sonHaricTutulanSiteler.push(deger); }
    window.chrome.webview.postMessage("set_exemptsites|" + sonHaricTutulanSiteler.join("|"));
    girdi.value = "";
});

// "Her indirmede konum sor" anahtarı değiştiğinde native tarafa kaydedilir
document.getElementById("anahtar-indirme-konum-sor").addEventListener("change", function (olay) {
    window.chrome.webview.postMessage("set_askdownloadlocation|" + (olay.target.checked ? "evet" : "hayir"));
});

// Enerji tasarrufu modu anahtarı değiştiğinde native tarafa kaydedilir
document.getElementById("anahtar-enerji-tasarrufu").addEventListener("change", function (olay) {
    window.chrome.webview.postMessage("set_energysaver|" + (olay.target.checked ? "evet" : "hayir"));
});

// Önceki oturumu geri yükleme anahtarı değiştiğinde native tarafa kaydedilir
document.getElementById("anahtar-oturum-geri-yukle").addEventListener("change", function (olay) {
    window.chrome.webview.postMessage("set_restoresession|" + (olay.target.checked ? "evet" : "hayir"));
});

// Sızıntı kontrolü anahtarı değiştiğinde native tarafa kaydedilir, şifre listesi de yeni duruma göre yeniden çizilir
document.getElementById("anahtar-sizinti-kontrolu").addEventListener("change", function (olay) {
    sizintiKontroluAcikMi = olay.target.checked;
    window.chrome.webview.postMessage("set_breachcheck|" + (olay.target.checked ? "evet" : "hayir"));
    window.chrome.webview.postMessage("get_passwords"); // Liste, butonun görünürlüğü güncellensin diye tekrar istenir
});

// Yeni profil oluşturup geçme butonu
document.getElementById("buton-yeni-profil-olustur").addEventListener("click", function () {
    const ad = document.getElementById("girdi-yeni-profil-adi").value.trim();
    if (!ad) { return; }
    if (!/^[a-zA-Z0-9_\- ]+$/.test(ad)) { alert("Profil adı sadece harf, rakam, boşluk, tire ve alt çizgi içerebilir."); return; }
    window.chrome.webview.postMessage("switch_profile|" + ad.replace(/ /g, "_"));
});

// Native taraftan profil listesi geldiğinde ekrana çizen fonksiyon
function profilListesiniCiz(profiller, aktifProfil) {
    const kapsayici = document.getElementById("profiller-listesi");
    kapsayici.innerHTML = "";

    const tumProfiller = ["(Varsayılan)"].concat(profiller); // Profilsiz (varsayılan) mod da listede bir seçenek olarak gösterilir
    tumProfiller.forEach(function (ad) {
        const gercekAd = ad === "(Varsayılan)" ? "" : ad;
        const satir = document.createElement("div");
        satir.className = "liste-satiri";

        const anaMetin = document.createElement("div");
        anaMetin.className = "ana-metin";
        anaMetin.textContent = ad + (gercekAd === aktifProfil ? " (şu an aktif)" : "");
        satir.appendChild(anaMetin);

        if (gercekAd !== aktifProfil) {
            const gecButonu = document.createElement("button");
            gecButonu.textContent = "Bu Profile Geç";
            gecButonu.addEventListener("click", function () {
                window.chrome.webview.postMessage("switch_profile|" + gercekAd);
            });
            satir.appendChild(gecButonu);
        }

        kapsayici.appendChild(satir);
    });
}

window.chrome.webview.postMessage("get_profiles"); // Sayfa yüklenir yüklenmez, profil listesi istenir

// Site içeriği dili/yazım denetimi sözlüğü seçimi, değiştirildiği anda native tarafa kaydedilir (etkili olması için yeniden başlatma gerekir)
document.getElementById("secim-tarayici-dili").addEventListener("change", function (olay) {
    window.chrome.webview.postMessage("set_language|" + olay.target.value);
});

// Varsayılan tarayıcı yapma butonu, Windows'un kendi ayar sayfasını açar
document.getElementById("buton-varsayilan-tarayici-ayarlar").addEventListener("click", function () {
    window.chrome.webview.postMessage("open_default_apps_settings");
});

// Bellek tasarrufu modu eşiği, değiştirildiği anda native tarafa kaydedilir
document.getElementById("secim-uyku-esigi").addEventListener("change", function (olay) {
    window.chrome.webview.postMessage("set_sleepthreshold|" + olay.target.value);
});

// Yeni sekme arka plan resmi seçme/kaldırma butonları
document.getElementById("buton-arkaplan-sec").addEventListener("click", function () {
    window.chrome.webview.postMessage("pick_newtab_background");
});
document.getElementById("buton-arkaplan-kaldir").addEventListener("click", function () {
    window.chrome.webview.postMessage("clear_newtab_background");
});

// Kısayolları özelleştirme formu, native tarafa kaydeder, önce aynı harfin iki eyleme birden atanmadığından emin olunur
document.getElementById("buton-kisayol-kaydet").addEventListener("click", function () {
    const alanlar = {
        newtab: document.getElementById("kisayol-yenisekme").value.toUpperCase(),
        closetab: document.getElementById("kisayol-sekmekapat").value.toUpperCase(),
        duplicate: document.getElementById("kisayol-cogalt").value.toUpperCase(),
        print: document.getElementById("kisayol-yazdir").value.toUpperCase(),
        find: document.getElementById("kisayol-sayfadabul").value.toUpperCase(),
        reload: document.getElementById("kisayol-yenile").value.toUpperCase(),
        address: document.getElementById("kisayol-adres").value.toUpperCase()
    };

    const degerler = Object.values(alanlar);
    if (degerler.some(function (v) { return !v; })) {
        alert("Lütfen her kısayol için tek bir harf girin, boş bırakmayın.");
        return;
    }
    if (new Set(degerler).size !== degerler.length) { // Aynı harfin iki farklı eyleme atanmış olup olmadığı, bir Set'in boyutuyla kontrol edilir
        alert("Aynı harf birden fazla kısayola atanamaz, lütfen her birine farklı bir harf seçin.");
        return;
    }

    window.chrome.webview.postMessage("set_shortcuts|" + JSON.stringify(alanlar));
});

// Bu fonksiyon, büyük/küçük harf, rakam ve sembol içeren, 16 karakterlik güçlü ve rastgele bir şifre üretir, Math.random KULLANMAZ, tarayıcının kendi kriptografik rastgele sayı üretecini (crypto.getRandomValues) kullanır
function guvenliSifreUret() {
    const karakterKumesi = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789!@#$%^&*-_=+"; // Kafa karıştırıcı karakterler (0/O, 1/l/I gibi) kasıtlı olarak kümeden çıkarılmıştır
    const rastgeleBaytlar = new Uint32Array(16); // 16 karakterlik şifre için 16 rastgele sayı üretilecek
    crypto.getRandomValues(rastgeleBaytlar); // Tarayıcının kendi güvenli rastgele sayı üreteci kullanılır
    let sonuc = "";
    for (let i = 0; i < rastgeleBaytlar.length; i++) {
        sonuc += karakterKumesi[rastgeleBaytlar[i] % karakterKumesi.length]; // Her rastgele sayı, karakter kümesinden bir karaktere eşlenir
    }
    return sonuc;
}

// Şifre üretme butonu, üretilen şifreyi doğrudan şifre kutusuna yazar, kullanıcı isterse "Ekle"ye basmadan önce kutudan düzenleyebilir
document.getElementById("buton-sifre-uret").addEventListener("click", function () {
    const girdiSifre = document.getElementById("girdi-sifre");
    girdiSifre.value = guvenliSifreUret();
    girdiSifre.type = "text"; // Kullanıcı üretilen şifreyi hemen görüp kontrol edebilsin diye kısa süreliğine düz metin olarak gösterilir
    setTimeout(function () { girdiSifre.type = "password"; }, 2500); // Kısa bir süre sonra tekrar gizlenir
});

// Şifre ekleme formu
document.getElementById("buton-sifre-ekle").addEventListener("click", function () {
    const site = document.getElementById("girdi-site").value; // Site adı okunur
    const kullanici = document.getElementById("girdi-kullanici").value; // Kullanıcı adı okunur
    const sifre = document.getElementById("girdi-sifre").value; // Şifre okunur

    if (!site || !sifre) { // Eğer site veya şifre boşsa
        alert("Lütfen en azından site ve şifre alanlarını doldurun."); // Kullanıcı uyarılır
        return; // İşlem durdurulur
    }

    const veri = { site: site, username: kullanici, password: sifre }; // Gönderilecek veri hazırlanır
    window.chrome.webview.postMessage("add_password|" + JSON.stringify(veri)); // Komut, native tarafa gönderilir

    document.getElementById("girdi-site").value = ""; // Form alanları temizlenir
    document.getElementById("girdi-kullanici").value = "";
    document.getElementById("girdi-sifre").value = "";
});

// Kişisel bilgi profili formu, native tarafa kaydeder
document.getElementById("buton-profil-kaydet").addEventListener("click", function () {
    const veri = {
        adSoyad: document.getElementById("girdi-profil-adsoyad").value,
        eposta: document.getElementById("girdi-profil-eposta").value,
        telefon: document.getElementById("girdi-profil-telefon").value,
        adres: document.getElementById("girdi-profil-adres").value
    };
    window.chrome.webview.postMessage("set_autofillprofile|" + JSON.stringify(veri));
});

// Kart ekleme formu
document.getElementById("buton-kart-ekle").addEventListener("click", function () {
    const sahibi = document.getElementById("girdi-kart-sahibi").value;
    const numara = document.getElementById("girdi-kart-numara").value;
    const tarih = document.getElementById("girdi-kart-tarih").value;

    if (!sahibi || !numara) {
        alert("Lütfen en azından kart sahibi ve kart numarasını doldurun.");
        return;
    }

    window.chrome.webview.postMessage("add_card|" + JSON.stringify({ holder: sahibi, number: numara, expiry: tarih }));

    document.getElementById("girdi-kart-sahibi").value = "";
    document.getElementById("girdi-kart-numara").value = "";
    document.getElementById("girdi-kart-tarih").value = "";
});

// Kart listesini, native taraftan gelen kayıtlarla ekrana çizen fonksiyon, gösterilen kart numaralarını geçici olarak hatırlar
let gosterilenKartlar = {}; // Anahtar "kartSahibi|eklenmeZamani" biçiminde, değer çözülmüş düz metin kart numarasıdır, sadece kullanıcı "Göster" dedikten sonra doldurulur

function kartListesiniCiz(liste) {
    const kapsayici = document.getElementById("kart-listesi");
    kapsayici.innerHTML = "";

    liste.forEach(function (kayit) {
        const anahtar = kayit.kartSahibi + "|" + kayit.eklenmeZamani;
        const satir = document.createElement("div");
        satir.className = "liste-satiri";

        const anaMetin = document.createElement("div");
        anaMetin.className = "ana-metin";
        const gosterilecekNumara = gosterilenKartlar[anahtar];
        anaMetin.innerHTML = kayit.kartSahibi + " (" + (kayit.sonKullanma || "—") + ")<div class=\"ikincil-metin\">" + (gosterilecekNumara ? ("Numara: " + gosterilecekNumara) : "•••• •••• •••• ••••") + "</div>";

        const gosterButonu = document.createElement("button");
        gosterButonu.textContent = gosterilecekNumara ? "Gizle" : "Göster";
        gosterButonu.addEventListener("click", function () {
            if (gosterilenKartlar[anahtar]) {
                delete gosterilenKartlar[anahtar];
                kartListesiniCiz(liste);
            } else {
                window.chrome.webview.postMessage("reveal_card|" + JSON.stringify({ holder: kayit.kartSahibi, addedAt: kayit.eklenmeZamani }));
            }
        });

        const kopyalaButonu = document.createElement("button");
        kopyalaButonu.textContent = "Kopyala";
        kopyalaButonu.addEventListener("click", function () {
            if (gosterilecekNumara) { navigator.clipboard.writeText(gosterilecekNumara); }
            else { alert("Kopyalamadan önce önce \"Göster\" ile kart numarasını görüntüleyin."); }
        });

        const silButonu = document.createElement("button");
        silButonu.textContent = "Sil";
        silButonu.addEventListener("click", function () {
            window.chrome.webview.postMessage("remove_card|" + JSON.stringify({ holder: kayit.kartSahibi, addedAt: kayit.eklenmeZamani }));
        });

        satir.appendChild(anaMetin);
        satir.appendChild(gosterButonu);
        satir.appendChild(kopyalaButonu);
        satir.appendChild(silButonu);
        kapsayici.appendChild(satir);
    });

    aramaFiltresiUygula();
}

// Şifre listesini, native taraftan gelen kayıtlarla ekrana çizen fonksiyon, gösterilen şifreleri geçici olarak hatırlar
let gosterilenSifreler = {}; // Anahtar "site|kullanıcı" biçiminde, değer çözülmüş düz metin şifredir, sadece kullanıcı "Göster" dedikten sonra doldurulur
let sizintiKontroluAcikMi = false; // Ayarlardan okunan, "Sızıntı Kontrolü" butonunun gösterilip gösterilmeyeceğini belirleyen değer
let sizintiSonuclari = {}; // Anahtar "site|kullanıcı", değer native taraftan gelen son sızıntı kontrolü sonucu metni
let sonSifreListesi = []; // En son native taraftan gelen tam şifre listesi, sızıntı sonucu geldiğinde listeyi yeniden çizmek için saklanır

function sifreListesiniCiz(liste) {
    const kapsayici = document.getElementById("sifre-listesi"); // Listenin çizileceği kapsayıcı
    kapsayici.innerHTML = ""; // Önceki içerik temizlenir

    liste.forEach(function (kayit) { // Her şifre kaydı için
        const anahtar = kayit.site + "|" + kayit.username; // Bu kayda özel benzersiz bir anahtar oluşturulur
        const satir = document.createElement("div"); // Yeni bir satır öğesi oluşturulur
        satir.className = "liste-satiri"; // Satıra stil sınıfı atanır

        const anaMetin = document.createElement("div"); // Site ve kullanıcı adını gösterecek metin öğesi
        anaMetin.className = "ana-metin"; // Stil sınıfı atanır
        const gosterilecekSifre = gosterilenSifreler[anahtar]; // Bu kayıt daha önce çözülüp gösterildiyse, değeri alınır
        anaMetin.innerHTML = kayit.site + " (" + kayit.username + ")<div class=\"ikincil-metin\">" + (gosterilecekSifre ? ("Şifre: " + gosterilecekSifre) : "••••••••") + "</div>"; // Şifre gösterilmediyse noktalarla temsil edilir

        const gosterButonu = document.createElement("button"); // "Göster" butonu oluşturulur
        gosterButonu.textContent = gosterilecekSifre ? "Gizle" : "Göster"; // Buton metni, şifrenin gösterilip gösterilmediğine göre değişir
        gosterButonu.addEventListener("click", function () {
            if (gosterilenSifreler[anahtar]) { // Eğer şifre zaten gösteriliyorsa
                delete gosterilenSifreler[anahtar]; // Gösterilen şifre hafızadan silinir
                sifreListesiniCiz(liste); // Liste, şifre tekrar gizlenmiş olarak yeniden çizilir
            } else { // Eğer şifre henüz gösterilmiyorsa
                window.chrome.webview.postMessage("reveal_password|" + JSON.stringify({ site: kayit.site, username: kayit.username })); // Native taraftan, Windows doğrulamasından sonra şifrenin çözülmesi istenir
            }
        });

        const silButonu = document.createElement("button"); // "Sil" butonu oluşturulur
        silButonu.textContent = "Sil"; // Buton metni ayarlanır
        silButonu.addEventListener("click", function () {
            window.chrome.webview.postMessage("remove_password|" + JSON.stringify({ site: kayit.site, username: kayit.username })); // Native tarafa, bu kaydın silinmesi komutu gönderilir
        });

        satir.appendChild(anaMetin); // Ana metin, satıra eklenir
        satir.appendChild(gosterButonu); // Göster/Gizle butonu, satıra eklenir

        if (sizintiKontroluAcikMi) { // Ayarlardan açıldıysa, her satıra bir sızıntı kontrolü butonu eklenir
            const sizintiButonu = document.createElement("button");
            const sizintiSonucMetni = sizintiSonuclari[anahtar];
            sizintiButonu.textContent = sizintiSonucMetni ? sizintiSonucMetni : "Sızıntı Kontrolü";
            sizintiButonu.title = "Şifrenin kendisi ağa gönderilmez, sadece özetinin ilk 5 karakteri Have I Been Pwned servisine gönderilir";
            sizintiButonu.addEventListener("click", function () {
                sizintiButonu.textContent = "Kontrol ediliyor…";
                window.chrome.webview.postMessage("check_breach|" + JSON.stringify({ site: kayit.site, username: kayit.username }));
            });
            satir.appendChild(sizintiButonu);
        }

        satir.appendChild(silButonu); // Sil butonu, satıra eklenir
        kapsayici.appendChild(satir); // Hazırlanan satır, listeye eklenir
    });

    aramaFiltresiUygula(); // Liste yeniden çizildiği için, durum çubuğu ve görünürlük bu yeni satırlara göre tazelenir
}

// Her izin türünün, listede gösterilecek doğal dildeki Türkçe karşılığı
const izinTuruGorunenAdlari = {
    konum: "Konum", kamera: "Kamera", mikrofon: "Mikrofon", bildirim: "Bildirim", otomatikoynatma: "Otomatik Oynatma"
};

// İzin listesini, native taraftan gelen kayıtlarla ekrana çizen fonksiyon
function izinListesiniCiz(liste) {
    const kapsayici = document.getElementById("izinler-listesi"); // Listenin çizileceği kapsayıcı
    kapsayici.innerHTML = ""; // Önceki içerik temizlenir

    liste.forEach(function (kayit) { // Her izin kaydı için
        const satir = document.createElement("div"); // Yeni bir satır öğesi oluşturulur
        satir.className = "liste-satiri"; // Satıra stil sınıfı atanır

        const anaMetin = document.createElement("div"); // Kaynak ve izin türünü gösterecek metin öğesi
        anaMetin.className = "ana-metin"; // Stil sınıfı atanır
        const turAdi = izinTuruGorunenAdlari[kayit.tur] || kayit.tur; // Türün Türkçe adı bulunur, tanınmayan bir türse olduğu gibi gösterilir
        const kararMetni = kayit.izinVerildi ? "İzin verildi" : "Engellendi"; // Kararın okunabilir hali
        anaMetin.innerHTML = kayit.kaynak + "<div class=\"ikincil-metin\">" + turAdi + " · " + kararMetni + "</div>"; // Kaynak ve karar, satırın içine yazılır

        const kaldirButonu = document.createElement("button"); // "Kaldır" butonu oluşturulur
        kaldirButonu.textContent = "Kaldır"; // Buton metni ayarlanır
        kaldirButonu.addEventListener("click", function () {
            window.chrome.webview.postMessage("remove_permission|" + JSON.stringify({ kaynak: kayit.kaynak, tur: kayit.tur })); // Native tarafa, bu kararın silinmesi komutu gönderilir
        });

        satir.appendChild(anaMetin); // Ana metin, satıra eklenir
        satir.appendChild(kaldirButonu); // Kaldır butonu, satıra eklenir
        kapsayici.appendChild(satir); // Hazırlanan satır, listeye eklenir
    });

    aramaFiltresiUygula(); // Liste yeniden çizildiği için, durum çubuğu ve görünürlük bu yeni satırlara göre tazelenir
}

// Yer imleri listesini, native taraftan gelen kayıtlarla ekrana çizen fonksiyon, sürükle-bırak ile sıralamayı ve klasöre taşımayı destekler
let sonYerImleriListesi = []; // En son native taraftan gelen tam yer imi listesi, arama kutusuna yazıldıkça tekrar native tarafa sormadan filtrelemek için saklanır
let suruklenenYerImiAdresi = null; // Şu anda sürüklenmekte olan yer iminin adresi, sürükleme yoksa boştur

function yerimleriListesiniCiz() {
    const aramaMetni = aramaKutusu.value.toLowerCase(); // Üstteki paylaşılan arama kutusuna yazılan metin okunur
    const kapsayici = document.getElementById("yerimleri-listesi"); // Listenin çizileceği kapsayıcı
    kapsayici.innerHTML = ""; // Önceki içerik temizlenir

    const filtrelenmis = sonYerImleriListesi.filter(function (kayit) { // Tam listeden, arama metnine uyan kayıtlar süzülür
        return kayit.baslik.toLowerCase().includes(aramaMetni) || kayit.adres.toLowerCase().includes(aramaMetni);
    });

    // Klasör adlarının, kullanıcının yeni bir klasöre taşırken seçebileceği bir listesi, mevcut tüm kayıtlardan (filtrelenmemiş) çıkarılır
    const mevcutKlasorler = Array.from(new Set(sonYerImleriListesi.map(function (k) { return k.klasor; }).filter(function (k) { return k; })));

    filtrelenmis.forEach(function (kayit) { // Her yer imi için bir satır oluşturulur
        const satir = document.createElement("div");
        satir.className = "liste-satiri";
        satir.draggable = true; // Sürükle-bırak ile sıralanabilmesi için
        satir.dataset.adres = kayit.adres;

        const anaMetin = document.createElement("div");
        anaMetin.className = "ana-metin";
        anaMetin.innerHTML = kayit.baslik + "<div class=\"ikincil-metin\">" + kayit.adres + (kayit.klasor ? " · 📁 " + kayit.klasor : "") + "</div>";

        const klasorSecici = document.createElement("select"); // Yer iminin hangi klasörde olduğunu gösteren ve değiştiren seçim kutusu
        klasorSecici.innerHTML = "<option value=\"\">Klasörsüz</option>" + mevcutKlasorler.map(function (k) { return "<option value=\"" + k + "\">" + k + "</option>"; }).join("") + "<option value=\"__yeni__\">+ Yeni klasör…</option>";
        klasorSecici.value = kayit.klasor || "";
        klasorSecici.addEventListener("change", function () {
            let yeniKlasor = klasorSecici.value;
            if (yeniKlasor === "__yeni__") { // Kullanıcı yeni bir klasör oluşturmak istiyorsa
                yeniKlasor = prompt("Yeni klasörün adı:", "");
                if (!yeniKlasor) { klasorSecici.value = kayit.klasor || ""; return; } // Boş geçildiyse eski değerine döndürülür
            }
            window.chrome.webview.postMessage("move_bookmark|" + JSON.stringify({ adres: kayit.adres, klasor: yeniKlasor }));
        });

        const silButonu = document.createElement("button");
        silButonu.textContent = "Kaldır";
        silButonu.addEventListener("click", function () {
            window.chrome.webview.postMessage("remove_bookmark|" + kayit.adres);
        });

        // Sürükle-bırak ile yeniden sıralama, chrome.js'deki sekme sıralamasıyla aynı yöntemi kullanır: sürüklenirken DOM anında yeniden sıralanır, bırakılınca native tarafa son sıra bildirilir
        satir.addEventListener("dragstart", function () {
            suruklenenYerImiAdresi = kayit.adres;
            setTimeout(function () { satir.classList.add("surukleniyor"); }, 0);
        });
        satir.addEventListener("dragend", function () {
            satir.classList.remove("surukleniyor");
            suruklenenYerImiAdresi = null;
        });
        satir.addEventListener("dragover", function (olay) {
            olay.preventDefault();
            if (!suruklenenYerImiAdresi || suruklenenYerImiAdresi === satir.dataset.adres) { return; }
            const suruklenenSatir = kapsayici.querySelector('.liste-satiri[data-adres="' + CSS.escape(suruklenenYerImiAdresi) + '"]');
            if (!suruklenenSatir) { return; }
            const dikdortgen = satir.getBoundingClientRect();
            const ustteOnceMi = (olay.clientY - dikdortgen.top) < dikdortgen.height / 2; // Fare, üzerine gelinen satırın üst mü alt yarısında
            kapsayici.insertBefore(suruklenenSatir, ustteOnceMi ? satir : satir.nextSibling);
        });
        satir.addEventListener("drop", function (olay) {
            olay.preventDefault();
            const yeniSira = Array.from(kapsayici.querySelectorAll(".liste-satiri")).map(function (s) { return s.dataset.adres; });
            window.chrome.webview.postMessage("reorder_bookmarks|" + yeniSira.join("|"));
        });

        satir.appendChild(anaMetin);
        satir.appendChild(klasorSecici);
        satir.appendChild(silButonu);
        kapsayici.appendChild(satir);
    });

    aramaFiltresiUygula(); // Liste yeniden çizildiği için, durum çubuğu ve görünürlük bu yeni satırlara göre tazelenir
}

// Okuma listesini, native taraftan gelen kayıtlarla ekrana çizen fonksiyon
let sonOkumaListesi = []; // En son native taraftan gelen tam okuma listesi

function okumaListesiniCiz() {
    const aramaMetni = aramaKutusu.value.toLowerCase(); // Üstteki paylaşılan arama kutusuna yazılan metin okunur
    const kapsayici = document.getElementById("okumalistesi-listesi"); // Listenin çizileceği kapsayıcı
    kapsayici.innerHTML = ""; // Önceki içerik temizlenir

    const filtrelenmis = sonOkumaListesi.filter(function (kayit) { // Tam listeden, arama metnine uyan kayıtlar süzülür
        return kayit.baslik.toLowerCase().includes(aramaMetni) || kayit.adres.toLowerCase().includes(aramaMetni);
    });

    for (let i = filtrelenmis.length - 1; i >= 0; i--) { // Liste, en yeni eklenen en üstte olacak şekilde ters sırada gezilir
        const kayit = filtrelenmis[i];
        const satir = document.createElement("div");
        satir.className = "liste-satiri";

        const anaMetin = document.createElement("div");
        anaMetin.className = "ana-metin";
        anaMetin.style.cursor = "pointer";
        anaMetin.innerHTML = kayit.baslik + "<div class=\"ikincil-metin\">" + kayit.adres + "</div>";
        anaMetin.addEventListener("click", function () {
            window.chrome.webview.postMessage("openbookmarkbar|" + kayit.adres); // Aynı doğrudan gezinme komutu, okuma listesi öğeleri için de kullanılır
        });

        const silButonu = document.createElement("button");
        silButonu.textContent = "Kaldır";
        silButonu.addEventListener("click", function (olay) {
            olay.stopPropagation();
            window.chrome.webview.postMessage("remove_readinglist|" + kayit.adres);
        });

        satir.appendChild(anaMetin);
        satir.appendChild(silButonu);
        kapsayici.appendChild(satir);
    }

    aramaFiltresiUygula(); // Liste yeniden çizildiği için, durum çubuğu ve görünürlük bu yeni satırlara göre tazelenir
}

// Bu fonksiyon, baytı okunabilir bir boyuta (KB/MB) çevirir
function boyutMetniOlustur(bayt) {
    if (!bayt || bayt <= 0) { return "0 KB"; }
    if (bayt < 1024 * 1024) { return Math.round(bayt / 1024) + " KB"; }
    return (bayt / (1024 * 1024)).toFixed(1) + " MB";
}

// Uzantı yükleme butonu
document.getElementById("buton-uzanti-yukle").addEventListener("click", function () {
    window.chrome.webview.postMessage("pick_extension_folder");
});

// Mağaza adresinden uzantı yükleme butonu
document.getElementById("buton-magaza-uzanti-yukle").addEventListener("click", function () {
    const adres = document.getElementById("girdi-magaza-adres").value.trim();
    if (!adres) { return; }
    document.getElementById("magaza-uzanti-durum").textContent = "İndiriliyor…";
    document.getElementById("buton-magaza-uzanti-yukle").disabled = true;
    window.chrome.webview.postMessage("load_extension_from_store|" + adres);
});

// Uzantı listesini, native taraftan gelen kayıtlarla ekrana çizen fonksiyon
function uzantilarListesiniCiz(liste) {
    const kapsayici = document.getElementById("uzantilar-listesi");
    kapsayici.innerHTML = "";

    if (liste.length === 0) {
        kapsayici.innerHTML = "<div class=\"liste-satiri\"><div class=\"ana-metin\">Henüz yüklü bir uzantı yok.</div></div>";
        return;
    }

    liste.forEach(function (kayit) {
        const satir = document.createElement("div");
        satir.className = "liste-satiri";

        const anaMetin = document.createElement("div");
        anaMetin.className = "ana-metin";
        anaMetin.innerHTML = kayit.name + "<div class=\"ikincil-metin\">" + (kayit.enabled ? "Etkin" : "Devre dışı") + "</div>";

        const durumButonu = document.createElement("button");
        durumButonu.textContent = kayit.enabled ? "Devre Dışı Bırak" : "Etkinleştir";
        durumButonu.addEventListener("click", function () {
            window.chrome.webview.postMessage((kayit.enabled ? "disable_extension|" : "enable_extension|") + kayit.id);
        });

        const silButonu = document.createElement("button");
        silButonu.textContent = "Kaldır";
        silButonu.addEventListener("click", function () {
            window.chrome.webview.postMessage("remove_extension|" + kayit.id);
        });

        satir.appendChild(anaMetin);
        satir.appendChild(durumButonu);
        satir.appendChild(silButonu);
        kapsayici.appendChild(satir);
    });
    aramaFiltresiUygula();
}

// Betik ekleme formu
document.getElementById("buton-betik-ekle").addEventListener("click", function () {
    const ad = document.getElementById("girdi-betik-ad").value;
    const desen = document.getElementById("girdi-betik-desen").value;
    const kod = document.getElementById("girdi-betik-kod").value;

    if (!ad || !desen || !kod) {
        alert("Lütfen betik adı, desen ve kod alanlarının hepsini doldurun.");
        return;
    }

    window.chrome.webview.postMessage("add_userscript|" + JSON.stringify({ name: ad, pattern: desen, code: kod }));

    document.getElementById("girdi-betik-ad").value = "";
    document.getElementById("girdi-betik-desen").value = "";
    document.getElementById("girdi-betik-kod").value = "";
});

// Kullanıcı betikleri listesini, native taraftan gelen kayıtlarla ekrana çizen fonksiyon
function betiklerListesiniCiz(liste) {
    const kapsayici = document.getElementById("betikler-listesi");
    kapsayici.innerHTML = "";

    if (liste.length === 0) {
        kapsayici.innerHTML = "<div class=\"liste-satiri\"><div class=\"ana-metin\">Henüz kayıtlı bir kullanıcı betiği yok.</div></div>";
        return;
    }

    liste.forEach(function (kayit) {
        const satir = document.createElement("div");
        satir.className = "liste-satiri";

        const anaMetin = document.createElement("div");
        anaMetin.className = "ana-metin";
        anaMetin.innerHTML = kayit.ad + "<div class=\"ikincil-metin\">" + kayit.desen + " · " + (kayit.etkinMi ? "Etkin" : "Devre dışı") + "</div>";

        const durumButonu = document.createElement("button");
        durumButonu.textContent = kayit.etkinMi ? "Devre Dışı Bırak" : "Etkinleştir";
        durumButonu.addEventListener("click", function () {
            window.chrome.webview.postMessage("toggle_userscript|" + kayit.id + "|" + (kayit.etkinMi ? "hayir" : "evet"));
        });

        const silButonu = document.createElement("button");
        silButonu.textContent = "Sil";
        silButonu.addEventListener("click", function () {
            window.chrome.webview.postMessage("remove_userscript|" + kayit.id);
        });

        satir.appendChild(anaMetin);
        satir.appendChild(durumButonu);
        satir.appendChild(silButonu);
        kapsayici.appendChild(satir);
    });
    aramaFiltresiUygula();
}

// İndirme listesini, native taraftan gelen kayıtlarla ekrana çizen fonksiyon
function indirmelerListesiniCiz(liste) {
    const kapsayici = document.getElementById("indirmeler-listesi");
    kapsayici.innerHTML = "";

    const durumMetinleri = { indiriliyor: "İndiriliyor", duraklatildi: "Duraklatıldı", tamamlandi: "Tamamlandı", kesildi: "Kesildi" };

    for (let i = liste.length - 1; i >= 0; i--) { // En yeni indirme en üstte gösterilir
        const kayit = liste[i];
        const satir = document.createElement("div");
        satir.className = "liste-satiri";

        const yuzde = kayit.total > 0 ? Math.round((kayit.received / kayit.total) * 100) : 0;

        const anaMetin = document.createElement("div");
        anaMetin.className = "ana-metin";
        anaMetin.innerHTML = kayit.name + "<div class=\"ikincil-metin\">" + (durumMetinleri[kayit.state] || kayit.state) + " · " + boyutMetniOlustur(kayit.received) + (kayit.total > 0 ? (" / " + boyutMetniOlustur(kayit.total) + " (" + yuzde + "%)") : "") + "</div>";
        satir.appendChild(anaMetin);

        if (kayit.state === "indiriliyor") {
            const duraklatButonu = document.createElement("button");
            duraklatButonu.textContent = "Duraklat";
            duraklatButonu.addEventListener("click", function () { window.chrome.webview.postMessage("download_action|" + kayit.id + "|pause"); });
            satir.appendChild(duraklatButonu);
        } else if (kayit.state === "duraklatildi") {
            const devamButonu = document.createElement("button");
            devamButonu.textContent = "Devam Et";
            devamButonu.addEventListener("click", function () { window.chrome.webview.postMessage("download_action|" + kayit.id + "|resume"); });
            satir.appendChild(devamButonu);
        }

        if (kayit.state === "indiriliyor" || kayit.state === "duraklatildi") {
            const iptalButonu = document.createElement("button");
            iptalButonu.textContent = "İptal";
            iptalButonu.addEventListener("click", function () { window.chrome.webview.postMessage("download_action|" + kayit.id + "|cancel"); });
            satir.appendChild(iptalButonu);
        }

        if (kayit.state === "tamamlandi") {
            const klasordeGosterButonu = document.createElement("button");
            klasordeGosterButonu.textContent = "Klasörde Göster";
            klasordeGosterButonu.addEventListener("click", function () { window.chrome.webview.postMessage("download_showfolder|" + kayit.id); });
            satir.appendChild(klasordeGosterButonu);
        }

        kapsayici.appendChild(satir);
    }

    if (liste.length === 0) {
        const bosSatir = document.createElement("div");
        bosSatir.className = "liste-satiri";
        bosSatir.innerHTML = "<div class=\"ana-metin\">Bu oturumda henüz bir indirme yapılmadı.</div>";
        kapsayici.appendChild(bosSatir);
    }

    aramaFiltresiUygula();
}

// Native taraftan bir mesaj geldiğinde bu fonksiyon çalışır
window.chrome.webview.addEventListener("message", function (olay) {
    let veri; // Gelen mesajın JSON'dan çözülmüş halini tutacak değişken
    try {
        veri = JSON.parse(olay.data); // Native tarafın gönderdiği JSON metni, bir JavaScript nesnesine çevrilir
    } catch (hata) {
        return; // Eğer mesaj geçerli bir JSON değilse, güvenli bir şekilde yok sayılır
    }

    if (veri.type === "settings") { // Güncel ayarlar geldiyse
        document.getElementById("secim-arama-motoru").value = veri.searchEngine; // Arama motoru seçimi, kayıtlı değere ayarlanır
        document.getElementById("girdi-ana-sayfa").value = veri.homepage; // Ana sayfa kutusu, kayıtlı değere ayarlanır
        document.getElementById("secim-kilitlenme-suresi").value = String(veri.lockMinutes); // Şifre kasası kilitlenme süresi seçimi, kayıtlı değere ayarlanır
        document.getElementById("secim-uyku-esigi").value = String(veri.sleepThresholdMinutes); // Bellek tasarrufu eşiği seçimi, kayıtlı değere ayarlanır
        document.getElementById("arkaplan-dosya-adi").textContent = veri.backgroundPath ? ("Seçili dosya: " + veri.backgroundPath.split("\\").pop()) : "Varsayılan desenli arka plan kullanılıyor"; // Tam yoldan sadece dosya adı gösterilir

        document.getElementById("kisayol-yenisekme").value = veri.shortcuts.newtab;
        document.getElementById("kisayol-sekmekapat").value = veri.shortcuts.closetab;
        document.getElementById("kisayol-cogalt").value = veri.shortcuts.duplicate;
        document.getElementById("kisayol-yazdir").value = veri.shortcuts.print;
        document.getElementById("kisayol-sayfadabul").value = veri.shortcuts.find;
        document.getElementById("kisayol-yenile").value = veri.shortcuts.reload;
        document.getElementById("kisayol-adres").value = veri.shortcuts.address;
        seciliVurguRengi = veri.theme.accent; // Seçili renk, kayıtlı değere ayarlanır
        seciliYogunluk = veri.theme.density; // Seçili yoğunluk, kayıtlı değere ayarlanır
        seciliYaziBoyutu = veri.theme.fontSize; // Seçili yazı boyutu, kayıtlı değere ayarlanır
        gorunumSecimleriniGuncelle(veri.theme); // Görünüm panelindeki seçim göstergeleri güncellenir
        // Ayarlar sekmesi, kendi <html>'ine data-tema özelliğini hiç eklemiyordu, bu yüzden kullanıcı "Açık" seçse de panel tokens.css'teki varsayılan koyu zemini kullanıyordu
        if (veri.theme.mode === "acik") { document.documentElement.setAttribute("data-tema", "acik"); }
        else if (veri.theme.mode === "yuksekkontrast") { document.documentElement.setAttribute("data-tema", "yuksekkontrast"); }
        else { document.documentElement.removeAttribute("data-tema"); }

        document.getElementById("anahtar-cikista-temizle").checked = veri.exitClean; // Çıkışta otomatik temizleme anahtarı, kayıtlı değere ayarlanır
        document.querySelectorAll("#cikista-temizle-kategoriler input[data-cikis-kategori]").forEach(function (kutu) {
            kutu.checked = veri.exitCleanCategories.split(",").indexOf(kutu.dataset.cikisKategori) !== -1; // Her kategori kutusu, kayıtlı listede kendi adı geçiyorsa işaretlenir
        });
        sonHaricTutulanSiteler = veri.exemptSites ? veri.exemptSites.split("|").filter(Boolean) : []; // Hariç tutulan site listesi, "|" ile ayrılmış metinden diziye çevrilir
        haricTutulanSiteleriCiz(); // Liste ekrana çizilir
        document.getElementById("anahtar-indirme-konum-sor").checked = veri.askDownloadLocation; // İndirme konumu sorma anahtarı, kayıtlı değere ayarlanır
        document.getElementById("anahtar-enerji-tasarrufu").checked = veri.energySaver; // Enerji tasarrufu modu anahtarı, kayıtlı değere ayarlanır
        document.getElementById("anahtar-oturum-geri-yukle").checked = veri.restoreSession; // Önceki oturumu geri yükleme anahtarı, kayıtlı değere ayarlanır
        document.getElementById("anahtar-sizinti-kontrolu").checked = veri.breachCheckEnabled; // Sızıntı kontrolü anahtarı, kayıtlı değere ayarlanır
        sizintiKontroluAcikMi = veri.breachCheckEnabled; // Şifre listesi çizilirken "Sızıntı Kontrolü" butonunun gösterilip gösterilmeyeceği bu değere bakar

        document.getElementById("anahtar-guvenli-gezinme").checked = veri.safeBrowsingEnabled; // Güvenli gezinme uyarısı anahtarı, kayıtlı değere ayarlanır
        document.getElementById("secim-tarayici-dili").value = veri.language || "tr-TR"; // Site içeriği dili seçimi, kayıtlı değere ayarlanır
        document.getElementById("anahtar-reklam-engelleme").checked = veri.adBlockEnabled; // Reklam engelleme anahtarı, kayıtlı değere ayarlanır
        document.querySelectorAll("#izleyici-seviye-secenekleri .segment").forEach(function (buton) {
            buton.classList.toggle("secili", buton.dataset.seviye === veri.trackerLevel); // İzleyici seviyesi seçenekleri, kayıtlı değere göre işaretlenir
        });
    }
    else if (veri.type === "history") { // Geçmiş listesi geldiyse
        sonGecmisListesi = veri.entries; // Gelen liste saklanır
        gecmisListesiniCiz(); // Liste ekrana çizilir
    }
    else if (veri.type === "errorlog") { // Hata günlüğü geldiyse
        sonHataListesi = veri.entries; // Gelen liste saklanır
        hataListesiniCiz(); // Liste ekrana çizilir
    }
    else if (veri.type === "passwords") { // Şifre listesi geldiyse
        sonSifreListesi = veri.entries; // Gelen liste saklanır, sızıntı sonucu geldiğinde tekrar kullanılır
        sifreListesiniCiz(veri.entries); // Liste ekrana çizilir
    }
    else if (veri.type === "breachresult") { // Bir şifrenin sızıntı/zayıflık kontrolü sonucu geldiyse
        const anahtar = veri.site + "|" + veri.username;
        let metin;
        if (veri.authFailed) { metin = "Doğrulama başarısız"; }
        else if (veri.breached) { metin = "⚠ " + veri.count + " sızıntıda bulundu"; }
        else if (veri.weak) { metin = "⚠ Zayıf şifre"; }
        else { metin = "✓ Sorun bulunamadı"; }
        sizintiSonuclari[anahtar] = metin;
        sifreListesiniCiz(sonSifreListesi); // Liste, güncel sonuç metniyle yeniden çizilir
    }
    else if (veri.type === "permissions") { // Kaydedilmiş izin kararları geldiyse
        izinListesiniCiz(veri.entries); // Liste ekrana çizilir
    }
    else if (veri.type === "bookmarks") { // Yer imi listesi geldiyse
        sonYerImleriListesi = veri.entries; // Gelen liste saklanır
        yerimleriListesiniCiz(); // Liste ekrana çizilir
    }
    else if (veri.type === "readinglist") { // Okuma listesi geldiyse
        sonOkumaListesi = veri.entries; // Gelen liste saklanır
        okumaListesiniCiz(); // Liste ekrana çizilir
    }
    else if (veri.type === "autofillprofile") { // Kişisel bilgi profili geldiyse
        document.getElementById("girdi-profil-adsoyad").value = veri.adSoyad;
        document.getElementById("girdi-profil-eposta").value = veri.eposta;
        document.getElementById("girdi-profil-telefon").value = veri.telefon;
        document.getElementById("girdi-profil-adres").value = veri.adres;
    }
    else if (veri.type === "downloads") { // İndirme listesi geldiyse
        indirmelerListesiniCiz(veri.entries);
    }
    else if (veri.type === "extensions") { // Uzantı listesi geldiyse
        uzantilarListesiniCiz(veri.entries);
    }
    else if (veri.type === "extension_load_result") { // Bir uzantı yükleme denemesinin sonucu geldiyse
        if (!veri.success) { alert("Uzantı yüklenemedi. Seçilen klasörün geçerli bir uzantı (manifest.json içeren) olduğundan emin olun."); }
    }
    else if (veri.type === "profiles") { // Profil listesi geldiyse
        profilListesiniCiz(veri.profiles, veri.active);
    }
    else if (veri.type === "store_extension_result") { // Mağaza adresinden uzantı yükleme denemesinin sonucu geldiyse
        const durumOgesi = document.getElementById("magaza-uzanti-durum");
        durumOgesi.textContent = veri.success ? "Uzantı başarıyla yüklendi." : ("Yükleme başarısız: " + (veri.error || "bilinmeyen hata"));
        document.getElementById("buton-magaza-uzanti-yukle").disabled = false;
    }
    else if (veri.type === "userscripts") { // Kullanıcı betikleri listesi geldiyse
        betiklerListesiniCiz(veri.entries);
    }
    else if (veri.type === "cards") { // Kart listesi geldiyse
        kartListesiniCiz(veri.entries);
    }
    else if (veri.type === "card_revealed") { // Bir kartın çözülmüş numarası geldiyse
        const anahtar = veri.holder + "|" + veri.addedAt;
        if (veri.number === null) {
            alert("Kart numarası gösterilemedi, Windows kimlik doğrulaması başarısız oldu.");
        } else {
            gosterilenKartlar[anahtar] = veri.number;
            window.chrome.webview.postMessage("get_cards"); // Liste, kart numarası açık şekilde görünecek biçimde yeniden çizilsin diye tekrar istenir
        }
    }
    else if (veri.type === "password_revealed") { // Bir şifrenin çözülmüş hali geldiyse
        const anahtar = veri.site + "|" + veri.username; // İlgili kaydın anahtarı oluşturulur
        if (veri.password === null) { // Eğer doğrulama başarısız olduysa
            alert("Şifre gösterilemedi, Windows kimlik doğrulaması başarısız oldu."); // Kullanıcı bilgilendirilir
        } else { // Eğer doğrulama başarılıysa
            gosterilenSifreler[anahtar] = veri.password; // Çözülen şifre, geçici olarak hafızada saklanır
            window.chrome.webview.postMessage("get_passwords"); // Liste, şifre açık şekilde görünecek biçimde yeniden çizilsin diye tekrar istenir
        }
    }
    else if (veri.type === "about") { // Hakkında bilgisi geldiyse, değerler artık satırın solundaki etiketin yanına yazıldığı için sadece değerin kendisi yazılır
        document.getElementById("hakkinda-ram").textContent = veri.ramMB + " MB"; // RAM bilgisi yazılır
        const saat = Math.floor(veri.uptimeSeconds / 3600); // Çalışma süresi saat cinsine çevrilir
        const dakika = Math.floor((veri.uptimeSeconds % 3600) / 60); // Kalan dakika hesaplanır
        document.getElementById("hakkinda-sure").textContent = saat + " sa " + dakika + " dk"; // Çalışma süresi yazılır
        document.getElementById("hakkinda-sekme").textContent = veri.tabCount; // Açık sekme sayısı yazılır
    }
});

// Sayfa yüklenir yüklenmez, Genel ve Görünüm panellerinin dolu gelmesi için güncel ayarlar native taraftan istenir
window.chrome.webview.postMessage("get_settings");

// Bu fonksiyon, adresin sonunda bir kategori belirtilmişse (örneğin settings.html#gecmis, hamburger menüsündeki kısayollardan gelir) doğrudan o kategoriye geçer
function hashtenKategoriyeGec() {
    const kategoriAdi = location.hash.replace("#", ""); // Baştaki # işareti kaldırılır
    if (kategoriAdi && document.getElementById("panel-" + kategoriAdi)) { // Bu isimde gerçekten bir panel varsa
        kategoriyeGec(kategoriAdi); // O kategoriye geçilir
    }
}
hashtenKategoriyeGec(); // Sayfa ilk açıldığında, adreste bir kategori varsa hemen uygulanır
window.addEventListener("hashchange", hashtenKategoriyeGec); // Ayarlar sekmesi zaten açıkken native taraf farklı bir kategoriyle tekrar gezindirirse (sayfa yeniden yüklenmeden) bu da yakalanır

aramaFiltresiUygula(); // Sayfa ilk açıldığında, varsayılan Genel panelinin durum çubuğu metni de hemen hazır olsun diye filtre bir kez boş sorguyla uygulanır
