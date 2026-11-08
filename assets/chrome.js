// Bu dosya, toolbar arayüzünün davranışını yönetir: sekme şeridinin nasıl çizileceğini (var olan sekmeleri silip baştan kurmak yerine sadece değişeni güncelleyerek, böylece açılış/kapanış animasyonları çalışabilir), butonlara tıklandığında ve adres çubuğuna bir şey yazılıp gönderildiğinde ne olacağını belirler.
// Bu dosya, native C++ tarafındaki BrowserWindow sınıfı ile window.chrome.webview.postMessage ve message olayı üzerinden konuşur.
// Native taraf, bir şey değiştiğinde (sayfa geçişi, yeni sekme, tema değişimi gibi) bir JSON metni gönderir, bu dosya bu metni JSON.parse ile okuyup arayüzü günceller.

const butonGeri = document.getElementById("buton-geri"); // Geri butonunun HTML öğesi alınır
const butonIleri = document.getElementById("buton-ileri"); // İleri butonunun HTML öğesi alınır
const butonYenile = document.getElementById("buton-yenile"); // Yenile butonunun HTML öğesi alınır
const butonAnaSayfa = document.getElementById("buton-anasayfa"); // Ana sayfa butonunun HTML öğesi alınır
const butonYildiz = document.getElementById("buton-yildiz"); // Yer imi yıldızı butonunun HTML öğesi alınır
const adresCubugu = document.getElementById("adres-cubugu"); // Adres çubuğunun HTML öğesi alınır
const butonKilit = document.getElementById("adres-cubugu-kilit"); // Adres çubuğundaki kilit ikonunun HTML öğesi alınır, tıklanınca site bilgisi/izinleri paneli açılır
const butonYeniSekme = document.getElementById("buton-yeni-sekme"); // Yeni sekme açma butonunun HTML öğesi alınır
const sekmeAramaGirdi = document.getElementById("sekme-arama-girdi"); // Sekme arama kutusunun HTML öğesi alınır, Ctrl+Shift+A ile açılır
const sekmelerKapsayici = document.getElementById("sekmeler-kapsayici"); // Sekme öğelerinin ekleneceği kapsayıcı alınır
const aktifSekmeGostergesi = document.getElementById("aktif-sekme-gostergesi"); // Aktif sekmenin altında kayan ince çizginin HTML öğesi alınır
const sekmeSeridi = document.getElementById("sekme-seridi"); // Sekme şeridinin tamamını saran dış kutu, boş alanına basılı tutulunca pencerenin sürüklenebilmesi için kullanılır
const butonKucult = document.getElementById("buton-kucult"); // Pencereyi küçülten butonun HTML öğesi alınır
const butonBuyut = document.getElementById("buton-buyut"); // Pencereyi büyüten/eski haline döndüren butonun HTML öğesi alınır
const butonKapat = document.getElementById("buton-kapat"); // Pencereyi kapatan butonun HTML öğesi alınır
const butonHamburger = document.getElementById("buton-hamburger"); // Üç yatay çizgiden oluşan menü butonu, menünün kendisi artık ayrı bir native pencerede (menu.html)
const yerImiCubugu = document.getElementById("yerimi-cubugu"); // En üst seviye yer imlerini gösteren ince çubuğun HTML öğesi alınır

let sonBilinenSekmeler = []; // Native taraftan en son gelen tam sekme listesi, arama kutusuna yazıldıkça yeniden çizmek için saklanır
let sonBilinenAktifId = -1; // Native taraftan en son gelen aktif sekme kimliği
let sonBilinenGruplar = []; // Native taraftan en son gelen tam sekme grubu listesi
let suruklenenSekmeId = null; // Şu anda sürüklenmekte olan sekmenin kimliği, sürükleme sırasında diğer sekmelerin üzerine gelindiğinde kullanılır, sürükleme yoksa boştur

butonGeri.addEventListener("click", function () {
    window.chrome.webview.postMessage("back"); // Geri butonuna tıklandığında, native tarafa düz metin olarak "back" komutu gönderilir
});

butonIleri.addEventListener("click", function () {
    window.chrome.webview.postMessage("forward"); // İleri butonuna tıklandığında, native tarafa "forward" komutu gönderilir
});

butonYenile.addEventListener("click", function () {
    window.chrome.webview.postMessage("reload"); // Yenile butonuna tıklandığında, native tarafa "reload" komutu gönderilir
});

butonAnaSayfa.addEventListener("click", function () {
    window.chrome.webview.postMessage("home"); // Ana sayfa butonuna tıklandığında, native tarafa "home" komutu gönderilir
});

butonYildiz.addEventListener("click", function () {
    window.chrome.webview.postMessage("star"); // Yıldız butonuna tıklandığında, native tarafa güncel sayfayı yer imlerine ekleyip çıkaracak "star" komutu gönderilir
});

// Kilit ikonu paneli artık ayrı bir native pencerede (site-paneli.html) gösterildiği için, buton sadece native tarafa "aç/kapat" komutu gönderir
// Panelin boyutu, konumu, görünürlüğü ve dışarı tıklanınca otomatik kapanması tamamen native tarafta (BrowserWindow.cpp) yönetilir, hamburger menüsüyle aynı mantık
butonKilit.addEventListener("click", function (olay) {
    olay.stopPropagation();
    butonKilit.setAttribute("aria-expanded", butonKilit.getAttribute("aria-expanded") !== "true"); // Komut bir toggle olduğu için, görünür durum da aynı mantıkla tersine çevrilir
    window.chrome.webview.postMessage("togglesitepaneli");
});

butonYeniSekme.addEventListener("click", function () {
    window.chrome.webview.postMessage("newtab"); // Yeni sekme butonuna tıklandığında, native tarafa "newtab" komutu gönderilir
});

butonKucult.addEventListener("click", function () {
    window.chrome.webview.postMessage("winminimize"); // Küçült butonuna tıklandığında, native tarafa pencereyi küçültme komutu gönderilir
});

butonBuyut.addEventListener("click", function () {
    window.chrome.webview.postMessage("winmaximize"); // Büyüt butonuna tıklandığında, native tarafa pencereyi büyütme/eski haline döndürme komutu gönderilir
});

butonKapat.addEventListener("click", function () {
    window.chrome.webview.postMessage("winclose"); // Kapat butonuna tıklandığında, native tarafa pencereyi kapatma komutu gönderilir
});

// Hamburger menü paneli artık ayrı bir native pencerede (menu.html) gösterildiği için, buton sadece native tarafa "aç/kapat" komutu gönderir
// Panelin boyutu, konumu, görünürlüğü ve dışarı tıklanınca otomatik kapanması tamamen native tarafta (BrowserWindow.cpp) yönetilir
butonHamburger.addEventListener("click", function (olay) {
    olay.stopPropagation();
    butonHamburger.setAttribute("aria-expanded", butonHamburger.getAttribute("aria-expanded") !== "true"); // Komut bir toggle olduğu için, görünür durum da aynı mantıkla tersine çevrilir
    window.chrome.webview.postMessage("togglemenu");
});

// Windows'un kendi başlık çubuğu kaldırıldığı için, sekme şeridinin boş (bir sekmeye veya butona denk gelmeyen) kısmına basılı tutup sürüklemek, pencereyi taşımalıdır
// Not: WM_NCLBUTTONDOWN ile başlatılan native sürükleme fareyi hemen Windows'a kaptırdığı için, tarayıcı ikinci tıklamayı asla "dblclick" olarak göremez
// Bu yüzden çift tıklama, native dblclick olayı yerine burada mousedown'ın olay.detail (tıklama sayısı) bilgisine bakılarak yakalanır
sekmeSeridi.addEventListener("mousedown", function (olay) {
    const bosAlanaTiklandiMi = (olay.target === sekmeSeridi || olay.target === sekmelerKapsayici); // Tıklanan yerin gerçekten boş arka plan olup olmadığı, bir sekmeye veya butona denk gelmediği kontrol edilir
    if (bosAlanaTiklandiMi && olay.button === 0) { // Eğer boş alana sol tuşla basıldıysa
        if (olay.detail >= 2) { // Bu, kısa süre içindeki ikinci (veya daha fazla) tıklamaysa, yani bir çift tıklamaysa
            window.chrome.webview.postMessage("winmaximize"); // Native tarafa, büyütme/eski haline döndürme komutu gönderilir, sürükleme başlatılmaz
        } else {
            window.chrome.webview.postMessage("windragstart"); // Tek tıklamaysa, native tarafa pencere sürüklemesini başlatma komutu gönderilir
        }
    }
});

// Bu fonksiyon, sekme arama kutusunu açar veya kapatır, kapatılınca aranan metin temizlenir ve tüm sekmeler tekrar görünür yapılır
function sekmeAramaKutusunuAcKapat() {
    const suankiDurum = sekmeAramaGirdi.classList.contains("gizli"); // Kutunun şu anda gizli olup olmadığı kontrol edilir
    sekmeAramaGirdi.classList.toggle("gizli", !suankiDurum); // Kutunun görünürlüğü tersine çevrilir
    if (suankiDurum) { // Eğer kutu şimdi açılıyorsa
        sekmeAramaGirdi.focus(); // Kutuya odaklanılır, kullanıcı doğrudan yazmaya başlayabilir
    } else { // Eğer kutu şimdi kapanıyorsa
        sekmeAramaGirdi.value = ""; // Aranan metin temizlenir
        sekmeSeridiniCiz(sonBilinenAktifId, sonBilinenSekmeler); // Sekme şeridi, filtre uygulanmadan tekrar çizilir
    }
}

sekmeAramaGirdi.addEventListener("input", function () {
    sekmeSeridiniCiz(sonBilinenAktifId, sonBilinenSekmeler); // Kullanıcı arama kutusuna her yazdığında, sekme şeridi güncel arama metnine göre yeniden çizilir
});

sekmeAramaGirdi.addEventListener("keydown", function (olay) {
    if (olay.key === "Escape") { // Kullanıcı arama kutusundayken Escape tuşuna bastıysa
        sekmeAramaKutusunuAcKapat(); // Arama kutusu kapatılır
    }
});

// Bu fonksiyon, sadece rakam, nokta, dört işlem ve parantezlerden oluşan basit bir matematik ifadesini güvenle hesaplar, eval veya Function KULLANMAZ, kendi küçük ayrıştırıcısını kullanır
// Geçersiz bir ifade verilirse (yanlış yazım, sıfıra bölme gibi) null döndürür, çağıran taraf bunu "bu bir matematik ifadesi değil" olarak yorumlar
function basitMatematikIfadesiniHesapla(metin) {
    const temizMetin = metin.trim(); // Baştaki ve sondaki boşluklar atılır
    if (!/^[0-9.+\-*/() ]+$/.test(temizMetin)) { return null; } // İzin verilen karakterler dışında bir şey varsa (harf, adres gibi), hiç denenmez
    if (!/[+\-*/]/.test(temizMetin.replace(/^-/, ""))) { return null; } // En baştaki eksi işareti hariç, içinde hiç işlem işareti yoksa (sadece "1984" gibi düz bir sayıysa), bu bir hesaplama değil muhtemelen bir yıl/port numarasıdır, dokunulmaz

    let konum = 0; // Ayrıştırıcının metin içinde şu an baktığı karakterin konumu

    function bosluklariAtla() { while (temizMetin[konum] === " ") { konum++; } }

    function sayiOku() { // Şu anki konumdan başlayan bir ondalıklı sayıyı okur
        bosluklariAtla();
        const baslangic = konum;
        while (konum < temizMetin.length && /[0-9.]/.test(temizMetin[konum])) { konum++; }
        const parcaMetni = temizMetin.slice(baslangic, konum);
        if (parcaMetni.length === 0) { throw new Error("sayı beklenirdi"); }
        if ((parcaMetni.match(/\./g) || []).length > 1) { throw new Error("geçersiz sayı, birden fazla nokta var"); } // "1.2.3" gibi birden fazla noktalı geçersiz bir sayının, parseFloat tarafından sessizce yanlış yorumlanması engellenir
        return parseFloat(parcaMetni);
    }

    function carpanOku() { // Parantezli bir ifadeyi, eksi işaretli bir değeri veya düz bir sayıyı okur
        bosluklariAtla();
        if (temizMetin[konum] === "-") { konum++; return -carpanOku(); } // Birli eksi işareti
        if (temizMetin[konum] === "(") {
            konum++; // Açılış parantezi atlanır
            const ifadeDegeri = toplamCarpimOku();
            bosluklariAtla();
            if (temizMetin[konum] !== ")") { throw new Error("kapanış parantezi bekleniyordu"); }
            konum++; // Kapanış parantezi atlanır
            return ifadeDegeri;
        }
        return sayiOku();
    }

    function carpimBolumOku() { // Çarpma ve bölme işlemlerini, soldan sağa doğru sırayla uygular
        let sonuc = carpanOku();
        while (true) {
            bosluklariAtla();
            if (temizMetin[konum] === "*") { konum++; sonuc *= carpanOku(); }
            else if (temizMetin[konum] === "/") { konum++; const bolen = carpanOku(); if (bolen === 0) { throw new Error("sıfıra bölme"); } sonuc /= bolen; }
            else { break; }
        }
        return sonuc;
    }

    function toplamCarpimOku() { // Toplama ve çıkarma işlemlerini, soldan sağa doğru sırayla uygular
        let sonuc = carpimBolumOku();
        while (true) {
            bosluklariAtla();
            if (temizMetin[konum] === "+") { konum++; sonuc += carpimBolumOku(); }
            else if (temizMetin[konum] === "-") { konum++; sonuc -= carpimBolumOku(); }
            else { break; }
        }
        return sonuc;
    }

    try {
        const sonuc = toplamCarpimOku();
        bosluklariAtla();
        if (konum !== temizMetin.length) { return null; } // Metin tam olarak tüketilmediyse (örneğin "2+2abc"), geçersiz sayılır
        if (!isFinite(sonuc)) { return null; } // Sonsuz veya tanımsız bir sonuç geçersiz sayılır
        return sonuc;
    } catch (hata) {
        return null; // Ayrıştırma sırasında herhangi bir hata, geçersiz ifade olarak yorumlanır
    }
}

const adresCubuguHesapSonucu = document.getElementById("adres-cubugu-hesap-sonucu"); // Hesap sonucunun gösterileceği küçük etiket

adresCubugu.addEventListener("input", function () { // Adres çubuğuna her yazıldığında, yazılan metnin bir matematik ifadesi olup olmadığı kontrol edilir
    const sonuc = basitMatematikIfadesiniHesapla(adresCubugu.value);
    if (sonuc === null) { // Geçerli bir matematik ifadesi değilse
        adresCubuguHesapSonucu.classList.add("gizli"); // Sonuç etiketi gizlenir
    } else { // Geçerli bir matematik ifadesiyse
        // Sonuç, gereksiz ondalık basamaklar olmadan, en fazla altı basamağa yuvarlanarak gösterilir
        adresCubuguHesapSonucu.textContent = "= " + parseFloat(sonuc.toFixed(6));
        adresCubuguHesapSonucu.classList.remove("gizli"); // Sonuç etiketi gösterilir
    }

    const sorgu = adresCubugu.value.trim(); // Öneri istemek için, baştaki/sondaki boşluklar atılmış hali kullanılır
    if (sorgu.length < 2) { // Çok kısa sorgularda (boş veya tek harf) anlamlı bir öneri çıkmaz, native tarafa boşuna istek gönderilmez
        window.chrome.webview.postMessage("close_addresssuggestions");
        return;
    }
    const kabRect = document.getElementById("adres-cubugu-kabi").getBoundingClientRect(); // Önerilerin, adres çubuğunun tam altında ve aynı genişlikte belirmesi için kabın ekrandaki konumu okunur
    // left/top/width burada bilerek metne çevrilip (String(...)) öyle gönderiliyor: JsonAlaniBul (native tarafta) sadece tırnaklı metin değerlerini okuyabiliyor, çıplak sayı gönderilirse hep boş okunup konum (0,0) sanılıyordu
    window.chrome.webview.postMessage("get_addresssuggestions|" + JSON.stringify({ query: sorgu, left: String(kabRect.left), top: String(kabRect.bottom), width: String(kabRect.width) }));
});

let enterBeklemedekiMetin = ""; // Enter'a basıldığında, native taraf klavyeyle seçili bir öneri olup olmadığını bildirene kadar yazılan metin burada saklanır

adresCubugu.addEventListener("keydown", function (olay) {
    if (olay.key === "Escape") { // Kullanıcı Escape'e basarsa, öneriler kapatılır
        window.chrome.webview.postMessage("close_addresssuggestions");
        return;
    }
    if (olay.key === "ArrowDown" || olay.key === "ArrowUp") { // Öneri paneli açıkken ok tuşları, imleci metin içinde taşımak yerine öneriler arasında gezinsin diye kullanılır
        olay.preventDefault(); // Adres çubuğunun imleci metin başına/sonuna taşıyan varsayılan davranışı engellenir
        window.chrome.webview.postMessage("addresssuggestion_navigate|" + (olay.key === "ArrowDown" ? "down" : "up"));
        return;
    }
    if (olay.key === "Enter") { // Kullanıcı adres çubuğundayken Enter tuşuna bastıysa
        if (!adresCubuguHesapSonucu.classList.contains("gizli")) { // Eğer o an geçerli bir matematik sonucu gösteriliyorsa
            adresCubugu.value = adresCubuguHesapSonucu.textContent.replace(/^=\s*/, ""); // Adres çubuğunun içeriği, sonucun kendisiyle değiştirilir
            adresCubuguHesapSonucu.classList.add("gizli"); // Sonuç etiketi, artık adres çubuğunun içeriğiyle aynı şeyi tekrarlamasın diye gizlenir
            adresCubugu.select(); // Sonuç, kullanıcı hemen üzerine yeni bir hesap yazabilsin diye seçili bırakılır
            window.chrome.webview.postMessage("close_addresssuggestions");
            return; // Bu basışta hiçbir yere gezinilmez, sadece sonuç adrese yazılır
        }
        // Öneri panelinde klavyeyle seçili bir satır olabileceğinden, önce native tarafa sorulur; native taraf seçili bir satır bulursa doğrudan oraya gider ve paneli kapatır,
        // bulamazsa "addresssuggestion_notconfirmed" ile geri bildirir, o zaman burada saklanan metin normal adres gezinmesi olarak gönderilir
        enterBeklemedekiMetin = adresCubugu.value;
        window.chrome.webview.postMessage("addresssuggestion_confirm");
    }
});

// Bu fonksiyon, verilen bir sekme nesnesi için (hem ilk oluşturulurken hem güncellenirken kullanılan) tek bir DOM öğesi hazırlar/günceller
function sekmeOgesiniDoldur(sekmeOgesi, sekme, aktifId, aramaMetni) {
    sekmeOgesi.dataset.sekmeId = sekme.id; // Öğenin hangi sekmeye ait olduğu, DOM üzerinde bir veri özelliği olarak tutulur, bir sonraki çizimde eşleştirme yapabilmek için

    sekmeOgesi.className = "sekme" + (sekme.id === aktifId ? " aktif" : "") + (sekme.suspended ? " uykuda" : "") + (sekme.private ? " gizli-sekme" : "") + (sekme.loading ? " yukleniyor" : "") + (sekme.pinned ? " sabitli" : ""); // Sekmenin tüm görsel durumları (aktif/uykuda/gizli/yükleniyor/sabitli), tek bir sınıf listesinde toplanır

    if (aramaMetni && !sekme.title.toLowerCase().includes(aramaMetni)) { // Eğer bir arama metni yazılmışsa ve bu sekmenin başlığı o metni içermiyorsa
        sekmeOgesi.classList.add("arama-ile-gizli"); // Bu sekme, arama sonucunda eşleşmediği için gizlenir
    }

    let gosterilecekBaslik = sekme.loading ? "Yükleniyor…" : sekme.title; // Sekme yükleniyorsa "Yükleniyor…" yazısı, değilse sayfanın başlığı gösterilir
    if (sekme.private) { gosterilecekBaslik = "🕶 " + gosterilecekBaslik; } // Gizli sekmelerin başlığının önüne küçük bir gizlilik işareti eklenir
    if (sekme.suspended) { gosterilecekBaslik = "💤 " + gosterilecekBaslik; } // Uykudaki sekmelerin başlığının önüne küçük bir uyku işareti eklenir

    // Öğe boşsa (ilk kez oluşturuluyorsa) iç yapı kurulur, zaten varsa sadece metinler güncellenir, böylece var olan DOM öğesi korunur ve animasyon kesintiye uğramaz
    if (!sekmeOgesi.dataset.kuruldu) {
        const faviconOgesi = document.createElement("div"); // Favicon yer tutucusu oluşturulur
        faviconOgesi.className = "sekme-favicon";

        const baslikOgesi = document.createElement("span"); // Başlık metin öğesi oluşturulur
        baslikOgesi.className = "sekme-baslik";

        const pinOgesi = document.createElement("span"); // Sabitleme işareti öğesi oluşturulur, sekmeye her zaman veya üzerine gelince görünür, tıklanınca sabitleme durumu tersine çevrilir
        pinOgesi.className = "sekme-pin";
        pinOgesi.textContent = "📌";
        pinOgesi.title = "Sekmeyi Sabitle/Sabitlemeyi Kaldır";

        const sesOgesi = document.createElement("span"); // Ses durumu işareti, sadece sekme ses çalarken veya sessize alınmışken görünür, tıklanınca sessize alma durumu tersine çevrilir
        sesOgesi.className = "sekme-ses";
        sesOgesi.title = "Sesi Kapat/Aç";

        const kapatOgesi = document.createElement("span"); // Kapatma işareti öğesi oluşturulur
        kapatOgesi.className = "sekme-kapat";
        kapatOgesi.textContent = "×";

        sekmeOgesi.appendChild(faviconOgesi);
        sekmeOgesi.appendChild(pinOgesi);
        sekmeOgesi.appendChild(baslikOgesi);
        sekmeOgesi.appendChild(sesOgesi);
        sekmeOgesi.appendChild(kapatOgesi);

        sekmeOgesi.addEventListener("click", function () {
            window.chrome.webview.postMessage("switchtab|" + sekmeOgesi.dataset.sekmeId); // Sekmenin herhangi bir yerine tıklanınca, native tarafa o sekmeye geçiş komutu gönderilir
        });
        kapatOgesi.addEventListener("click", function (olay) {
            olay.stopPropagation(); // Kapatma işaretine tıklamanın, üstteki sekme geçiş tıklamasını da tetiklemesi engellenir
            window.chrome.webview.postMessage("closetab|" + sekmeOgesi.dataset.sekmeId); // Native tarafa, bu sekmeyi kapatma komutu gönderilir
        });
        pinOgesi.addEventListener("click", function (olay) {
            olay.stopPropagation(); // Sabitleme işaretine tıklamanın, üstteki sekme geçiş tıklamasını da tetiklemesi engellenir
            window.chrome.webview.postMessage("pintab|" + sekmeOgesi.dataset.sekmeId); // Native tarafa, bu sekmenin sabitlenme durumunu tersine çevirme komutu gönderilir
        });
        sesOgesi.addEventListener("click", function (olay) {
            olay.stopPropagation(); // Ses işaretine tıklamanın, üstteki sekme geçiş tıklamasını da tetiklemesi engellenir
            window.chrome.webview.postMessage("mutetab|" + sekmeOgesi.dataset.sekmeId); // Native tarafa, bu sekmenin sessize alınma durumunu tersine çevirme komutu gönderilir
        });

        sekmeOgesi.draggable = true; // Sekmenin fare ile sürüklenip bırakılarak yeniden sıralanabilmesi için

        // Sürükle-bırak ile yeniden sıralama: sekme sürüklenmeye başlayınca kimliği hatırlanır, başka bir sekmenin üzerine gelindiğinde DOM'da anında yer değiştirilir, bırakılınca native tarafa son sıra bildirilir
        sekmeOgesi.addEventListener("dragstart", function (olay) {
            olay.dataTransfer.effectAllowed = "move";
            olay.dataTransfer.setData("text/plain", sekmeOgesi.dataset.sekmeId); // Firefox gibi bazı tarayıcılarda dataTransfer'e veri atanmazsa sürükleme hiç başlamaz, bu yüzden boş geçilmez
            suruklenenSekmeId = sekmeOgesi.dataset.sekmeId;
            sekmelerKapsayici.classList.add("surukleme-aktif"); // Sürükleme boyunca .sekme geçişleri kapatılır, aksi halde her insertBefore sonrası width geçişi yeniden tetiklenip titreme hissi verir
            setTimeout(function () { sekmeOgesi.classList.add("surukleniyor"); }, 0); // Bir an geciktirilir, aksi halde tarayıcının kendi sürükleme "hayalet görüntüsü" de saydam çekilir
        });
        sekmeOgesi.addEventListener("dragend", function () {
            sekmeOgesi.classList.remove("surukleniyor");
            sekmelerKapsayici.classList.remove("surukleme-aktif");
            suruklenenSekmeId = null;
        });
        sekmeOgesi.addEventListener("dragover", function (olay) {
            olay.preventDefault(); // Varsayılan olarak bir öğenin üzerine bırakma işlemi engellidir, bu satır izin verir
            if (!suruklenenSekmeId || suruklenenSekmeId === sekmeOgesi.dataset.sekmeId) { return; } // Sekme kendi üzerine sürükleniyorsa yapılacak bir şey yoktur
            const suruklenenOge = sekmelerKapsayici.querySelector('.sekme[data-sekme-id="' + suruklenenSekmeId + '"]');
            if (!suruklenenOge) { return; }
            // Sabitli bir sekme sadece diğer sabitli sekmelerin arasına, sabitsiz bir sekme de sadece diğer sabitsiz sekmelerin arasına taşınabilir, native taraf da bu grup bütünlüğünü zaten garanti eder
            if (suruklenenOge.classList.contains("sabitli") !== sekmeOgesi.classList.contains("sabitli")) { return; }
            const dikdortgen = sekmeOgesi.getBoundingClientRect();
            const ortadanOnceMi = (olay.clientX - dikdortgen.left) < dikdortgen.width / 2; // Fare, üzerine gelinen sekmenin sol mu sağ yarısında, dragend sonrası eklenecek konumu belirler
            sekmelerKapsayici.insertBefore(suruklenenOge, ortadanOnceMi ? sekmeOgesi : sekmeOgesi.nextSibling);
        });
        sekmeOgesi.addEventListener("drop", function (olay) {
            olay.preventDefault();
            sekmeSiralamasiniNativeyeGonder(); // Bırakma anındaki DOM sırası, kalıcı olması için native tarafa bildirilir
        });

        sekmeOgesi.addEventListener("mousedown", function (olay) {
            if (olay.button === 1) { // Fare tekerleğine (orta tuşa) basıldıysa
                olay.preventDefault(); // Windows'un orta tuşla gelen otomatik kaydırma (autoscroll) imlecinin belirmesi engellenir
            }
        });
        sekmeOgesi.addEventListener("auxclick", function (olay) { // Birincil (sol) tuş dışındaki bir tuşla tıklanınca tetiklenen olay
            if (olay.button === 1) { // Tıklanan tuş, fare tekerleği (orta tuş) ise
                window.chrome.webview.postMessage("closetab|" + sekmeOgesi.dataset.sekmeId); // Native tarafa, bu sekmeyi kapatma komutu gönderilir, tıpkı çarpı işaretine tıklanmış gibi
            }
        });
        sekmeOgesi.addEventListener("contextmenu", function (olay) { // Sağ tıklanınca, sekme gruplama bağlam menüsü açılır
            olay.preventDefault();
            olay.stopPropagation();
            const guncelSekme = sonBilinenSekmeler.find(function (s) { return String(s.id) === sekmeOgesi.dataset.sekmeId; });
            if (guncelSekme) { sekmeGrupMenusunuAc(guncelSekme, olay.clientX, olay.clientY); }
        });

        let onizlemeZamanlayicisi = null; // Fare sekmenin üzerine gelir gelmez değil, kısa bir süre orada kalırsa önizleme istenir, bu zamanlayıcının kimliği burada tutulur
        sekmeOgesi.addEventListener("mouseenter", function () {
            onizlemeZamanlayicisi = setTimeout(function () {
                sekmeOnizlemesiIste(sekmeOgesi); // Yarım saniye sonra hâlâ üzerindeyse, önizleme native taraftan istenir
            }, 500);
        });
        sekmeOgesi.addEventListener("mouseleave", function () {
            clearTimeout(onizlemeZamanlayicisi); // Fare hızla geçtiyse, henüz gönderilmemiş istek iptal edilir
            sekmeOnizlemesiniGizle(); // Açık olan önizleme varsa kapatılır
        });

        sekmeOgesi.dataset.kuruldu = "1"; // Bu öğenin iç yapısının artık kurulu olduğu işaretlenir
        sekmeOgesi.classList.add("sekme-giris"); // Sekme ilk kez oluşturulduğunda giriş animasyonu sınıfı eklenir, bir sonraki çizimde className tamamen yeniden yazılacağı için bu sınıf kendiliğinden düşer, animasyon tekrar tekrar oynamaz
    }

    sekmeOgesi.querySelector(".sekme-baslik").textContent = gosterilecekBaslik; // Başlık metni, her çizimde güncellenir

    const faviconOgesi = sekmeOgesi.querySelector(".sekme-favicon"); // Sekmenin site simgesi kutusu
    faviconOgesi.style.backgroundImage = sekme.favicon ? "url('" + sekme.favicon + "')" : ""; // Native taraf bu sekme için bir favicon bulduysa onu gösterir, bulamadıysa CSS'teki varsayılan program logosu kalır

    const sesOgesi = sekmeOgesi.querySelector(".sekme-ses"); // Ses durumu ikonu, sadece sekme ses çalıyorsa veya sessize alınmışsa görünür
    if (sekme.muted) { sesOgesi.textContent = "🔇"; sesOgesi.classList.add("gorunur"); }
    else if (sekme.playingAudio) { sesOgesi.textContent = "🔊"; sesOgesi.classList.add("gorunur"); }
    else { sesOgesi.classList.remove("gorunur"); }

    if (sekme.groupId) { // Sekme bir gruba aitse, grubun rengiyle ince bir alt çizgi gösterilir ve grup adı araç ipucu olarak eklenir
        const grup = sonBilinenGruplar.find(function (g) { return g.id === sekme.groupId; });
        sekmeOgesi.style.boxShadow = grup ? ("inset 0 -3px 0 " + grup.color) : "";
        sekmeOgesi.title = grup ? ("Grup: " + grup.name) : "";
    } else {
        sekmeOgesi.style.boxShadow = "";
        sekmeOgesi.title = "";
    }
}

// Bu fonksiyon, sekme şeridindeki GÜNCEL DOM sırasını okuyup, bu sırayı kalıcı olması için native tarafa bildirir, sürükle-bırak ile yeniden sıralama bittiğinde çağrılır
function sekmeSiralamasiniNativeyeGonder() {
    const idler = Array.from(sekmelerKapsayici.querySelectorAll(".sekme:not(.kapaniyor)")).map(function (oge) { return oge.dataset.sekmeId; }); // Kapanmakta olan sekmeler, native tarafta artık hiç var olmadığı için bu sıraya dahil edilmez
    window.chrome.webview.postMessage("reordertabs|" + idler.join(","));
}

// Bu fonksiyon, native taraftan gelen güncel sekme listesini kullanarak sekme şeridini DOM diff yöntemiyle günceller: var olan sekmeler silinip yeniden kurulmaz, sadece eklenen/çıkarılan sekmeler animasyonlu şekilde eklenir/kaldırılır
function sekmeSeridiniCiz(aktifId, sekmeler) {
    sonBilinenSekmeler = sekmeler; // Gelen liste, arama kutusuna yazıldıkça tekrar kullanılmak üzere saklanır
    sonBilinenAktifId = aktifId; // Gelen aktif sekme kimliği de saklanır

    const aramaMetni = sekmeAramaGirdi.value.toLowerCase(); // Arama kutusuna yazılmış olan metin, küçük harfe çevrilerek okunur

    const mevcutOgeler = new Map(); // Şu anda DOM'da bulunan sekme öğelerini, kimliklerine göre eşleyen bir harita
    sekmelerKapsayici.querySelectorAll(".sekme").forEach(function (oge) {
        mevcutOgeler.set(oge.dataset.sekmeId, oge);
    });

    const gorulenIdler = new Set(); // Bu çizimde native taraftan gelen tüm sekme kimliklerini tutan küme, hangi eski öğelerin artık fazla olduğunu bulmak için kullanılır

    let oncekiOge = aktifSekmeGostergesi; // Her yeni/var olan sekme öğesinin doğru sırada ekleneceği referans nokta, gösterge çizgisinden hemen sonra başlar
    sekmeler.forEach(function (sekme) { // Native taraftan gelen her sekme sırayla gezilir
        const idMetni = String(sekme.id); // Kimlik, harita anahtarıyla aynı türde (metin) karşılaştırılabilsin diye metne çevrilir
        gorulenIdler.add(idMetni); // Bu kimlik, görülenler kümesine eklenir

        let sekmeOgesi = mevcutOgeler.get(idMetni); // Bu kimliğe sahip bir öğe DOM'da zaten var mı diye bakılır
        const yeniMi = !sekmeOgesi; // Öğe yoksa, bu sekme yeni açılmış demektir

        if (yeniMi) { // Eğer bu sekme için DOM'da henüz bir öğe yoksa
            sekmeOgesi = document.createElement("div"); // Yeni bir öğe oluşturulur
            sekmeOgesi.style.opacity = "0"; // Önce görünmez başlar
            sekmeOgesi.style.transform = "scaleX(0.85)"; // Ve hafifçe daralmış başlar, animasyonun "büyüyerek gelme" hissi için
            sekmeOgesi.style.transition = "opacity 160ms ease, transform 160ms ease"; // Geçiş süresi tanımlanır
        }

        sekmeOgesiniDoldur(sekmeOgesi, sekme, aktifId, aramaMetni); // Öğenin içeriği (metin, sınıflar) dolduruluyor/güncelleniyor

        if (oncekiOge.nextSibling !== sekmeOgesi) { // Eğer öğe, kapsayıcıda doğru sırada değilse (sekmeler yeniden sıralanmış olabilir)
            sekmelerKapsayici.insertBefore(sekmeOgesi, oncekiOge.nextSibling); // Öğe, doğru konuma taşınır
        }
        oncekiOge = sekmeOgesi; // Bir sonraki sekmenin referans noktası, bu öğe olur

        if (yeniMi) { // Eğer bu öğe yeni eklendiyse
            requestAnimationFrame(function () { // Bir sonraki çizim karesinde (böylece "opacity:0" durumu tarayıcı tarafından gerçekten uygulanmış olur)
                sekmeOgesi.style.opacity = "1"; // Öğe, tam görünür hale animasyonla geçer
                sekmeOgesi.style.transform = "scaleX(1)"; // Öğe, tam genişliğine animasyonla büyür
            });
        }
    });

    // Kapatılan sekmeler, genişlik hesaplamasından ÖNCE işaretlenir, böylece sekmeGenislikleriniAyarla bu sekmeleri hiç saymaz ve kalan sekmeler, kapanan sekmenin boşalttığı yere doğru genişleyerek sanki sola kayıyormuş gibi görünür
    mevcutOgeler.forEach(function (oge, idMetni) { // DOM'da olan ama artık native taraftan gelen listede olmayan (kapatılmış) sekmeler için
        if (!gorulenIdler.has(idMetni)) { // Eğer bu öğenin kimliği, görülenler kümesinde yoksa
            oge.classList.add("kapaniyor"); // Genişlik hesaplamasından hariç tutulması için işaretlenir
            oge.style.width = "0px"; // Genişliği sıfıra iner, .sekme sınıfındaki mevcut width geçişi sayesinde bu küçülme de yumuşak olur, kalan sekmeler bu boşluğa doğru kayar
            oge.style.paddingLeft = "0px"; // İç boşluk da sıfırlanır, aksi halde sıfır genişlikte bile ince bir şerit görünür kalır
            oge.style.paddingRight = "0px";
            oge.style.opacity = "0"; // Aynı zamanda görünürlüğü de kaybolur
            setTimeout(function () { oge.remove(); }, 170); // Genişlik geçişi (160ms) bittikten hemen sonra, öğe DOM'dan tamamen kaldırılır
        }
    });

    sekmeGenislikleriniAyarla(sekmeler); // Kalan sekmelerin genişliği, kapsayıcıya tam sığacak şekilde yeniden hesaplanıp doğrudan atanır, CSS'in kendi otomatik hesaplamasına bırakılmaz, bu sayede hiçbir zaman taşma/kaydırma çubuğu oluşmaz

    aktifSekmeGostergesiniGuncelle(aktifId); // Aktif sekmenin altındaki kayan çizginin konumu, güncel aktif sekmeye göre yeniden hesaplanır
}

const SabitliSekmeGenisligi = 42; // Sabitlenmiş (pinned) sekmelerin, içerik sayısından bağımsız, her zaman kullanılan sabit genişliği

// Bu fonksiyon, açık olan sekmelerin listesine göre, her sekmenin genişliğini kapsayıcıya tam sığacak şekilde hesaplar ve doğrudan atar
// Sabitlenmiş sekmeler her zaman SabitliSekmeGenisligi kadar sabit yer kaplar, kalan genişlik sadece sabitlenmemiş sekmeler arasında paylaştırılır
// Bu hesaplama CSS'in kendi otomatik (flex) boyutlandırmasına bırakılmadığı için, yeni bir sekme eklenirken veya kapatılırken ara bir anda geçici taşma (ve dolayısıyla anlık kaydırma çubuğu) oluşması engellenir
function sekmeGenislikleriniAyarla(sekmeler) {
    if (sekmeler.length === 0) { // Eğer hiç sekme yoksa
        return; // Hesaplanacak bir şey yoktur
    }

    const sabitliSayisi = sekmeler.filter(function (s) { return s.pinned; }).length; // Sabitlenmiş sekmelerin sayısı
    const sabitsizSayisi = sekmeler.length - sabitliSayisi; // Kalan genişliği paylaşacak, sabitlenmemiş sekmelerin sayısı

    const butonGenisligi = butonYeniSekme.offsetWidth + 4; // Yeni sekme butonunun kendi genişliği, artı kapsayıcının genel boşluk (gap) değeri kadar pay
    const araBosluklar = (sekmeler.length - 1) * 4; // Sekmeler arasındaki boşlukların toplamı, sekme sayısından bir eksik kadar boşluk vardır
    const sabitliToplamGenislik = sabitliSayisi * SabitliSekmeGenisligi; // Sabitlenmiş sekmelerin birlikte kapladığı sabit toplam genişlik
    const kullanilabilirGenislik = Math.max(0, sekmelerKapsayici.clientWidth - butonGenisligi - araBosluklar - sabitliToplamGenislik); // Sadece sabitsiz sekmelere ayrılabilecek genişlik hesaplanır

    let sekmeGenisligi = sabitsizSayisi > 0 ? (kullanilabilirGenislik / sabitsizSayisi) : 0; // Bu genişlik, sabitsiz sekme sayısına eşit olarak bölünür
    sekmeGenisligi = Math.max(72, Math.min(240, sekmeGenisligi)); // Sonuç, en az 72 en fazla 240 piksel olacak şekilde sınırlanır (üst sınır, kullanıcının isteğiyle önceki 320 pikselin yüzde yetmiş beşine indirildi)

    sekmelerKapsayici.querySelectorAll(".sekme:not(.kapaniyor)").forEach(function (oge) { // Şu anda DOM'da bulunan, kapanmakta olanlar HARİÇ her sekme öğesi için
        oge.style.width = (oge.classList.contains("sabitli") ? SabitliSekmeGenisligi : sekmeGenisligi) + "px"; // Sabitlenmiş sekmeler kendi sabit genişliğini, diğerleri hesaplanan ortak genişliği kullanır
    });
}

// Bu fonksiyon, aktif sekmenin altındaki ince vurgu çizgisinin konumunu ve genişliğini, o sekmenin gerçek DOM konumuna göre hesaplayıp kaydırarak taşır
function aktifSekmeGostergesiniGuncelle(aktifId) {
    const aktifOge = sekmelerKapsayici.querySelector('.sekme[data-sekme-id="' + aktifId + '"]'); // Aktif sekmenin DOM öğesi bulunur
    if (!aktifOge) { // Eğer aktif bir sekme bulunamadıysa (örneğin henüz hiç sekme yoksa)
        aktifSekmeGostergesi.style.width = "0px"; // Çizgi görünmez hale getirilir
        return; // Fonksiyondan çıkılır
    }
    aktifSekmeGostergesi.style.width = aktifOge.offsetWidth + "px"; // Çizginin genişliği, aktif sekmenin genişliğiyle eşitlenir
    aktifSekmeGostergesi.style.transform = "translateX(" + aktifOge.offsetLeft + "px)"; // Çizgi, aktif sekmenin başladığı yatay konuma kaydırılır, bu kayma CSS transition sayesinde akıcı görünür
}

// Sekme önizleme balonu, sayfaya tek bir kere eklenir, tüm sekmeler bu tek öğeyi paylaşır
const sekmeOnizlemeBalonu = document.createElement("div");
sekmeOnizlemeBalonu.id = "sekme-onizleme-balonu";
const sekmeOnizlemeResmi = document.createElement("img");
sekmeOnizlemeBalonu.appendChild(sekmeOnizlemeResmi);
document.body.appendChild(sekmeOnizlemeBalonu);
let onizlenenSekmeId = null; // Şu anda önizlemesi istenen/gösterilen sekmenin kimliği, native taraftan cevap geldiğinde hâlâ aynı sekmenin üzerinde olunduğunu doğrulamak için tutulur

// Bu fonksiyon, verilen sekme öğesinin hemen altına önizleme balonunu konumlandırır ve native taraftan o sekmenin görüntüsünü ister
function sekmeOnizlemesiIste(sekmeOgesi) {
    onizlenenSekmeId = sekmeOgesi.dataset.sekmeId; // İstenen sekmenin kimliği hatırlanır
    const dikdortgen = sekmeOgesi.getBoundingClientRect(); // Sekmenin ekrandaki gerçek konumu ve boyutu öğrenilir
    sekmeOnizlemeBalonu.style.left = Math.round(Math.max(8, dikdortgen.left)) + "px"; // Balon, sekmenin solundan başlar ama pencerenin dışına taşmaz
    sekmeOnizlemeBalonu.style.top = Math.round(dikdortgen.bottom + 6) + "px"; // Balon, sekmenin hemen altında, küçük bir boşlukla başlar
    window.chrome.webview.postMessage("gettabpreview|" + onizlenenSekmeId); // Native taraftan, bu sekmenin anlık görüntüsü istenir
}

// Bu fonksiyon, açık olan önizleme balonunu gizler
function sekmeOnizlemesiniGizle() {
    onizlenenSekmeId = null; // Artık hiçbir sekme önizlenmiyor, native taraftan geç gelecek bir cevap da bu sayede görmezden gelinecek
    sekmeOnizlemeBalonu.classList.remove("gorunur");
}

// Sekme gruplama bağlam menüsü, sayfaya tek bir kere eklenir, sağ tıklanan sekmeye göre içeriği her seferinde yeniden doldurulur
const sekmeGrupMenusu = document.createElement("div");
sekmeGrupMenusu.id = "sekme-grup-menusu";
document.body.appendChild(sekmeGrupMenusu);

function sekmeGrupMenusunuGizle() {
    sekmeGrupMenusu.classList.remove("gorunur");
}
document.addEventListener("click", sekmeGrupMenusunuGizle); // Menünün dışına tıklanınca kapanır
document.addEventListener("scroll", sekmeGrupMenusunuGizle, true);

// Bu fonksiyon, verilen sekmeye sağ tıklandığında, o sekmenin güncel grup durumuna göre bir bağlam menüsü oluşturup fare konumunda gösterir
function sekmeGrupMenusunuAc(sekme, ekranX, ekranY) {
    sekmeGrupMenusu.innerHTML = "";

    const satirOlustur = function (metin, tikKomutu) {
        const satir = document.createElement("div");
        satir.className = "sekme-grup-menu-satiri";
        satir.textContent = metin;
        satir.addEventListener("click", function (olay) {
            olay.stopPropagation();
            tikKomutu();
            sekmeGrupMenusunuGizle();
        });
        sekmeGrupMenusu.appendChild(satir);
    };

    if (sekme.groupId) { // Sekme zaten bir gruptaysa, gruptan çıkarma ve grubu kapatma seçenekleri gösterilir
        satirOlustur("Gruptan Çıkar", function () { window.chrome.webview.postMessage("tabgroup_remove|" + sekme.id); });
        satirOlustur("Grubu Kapat", function () { window.chrome.webview.postMessage("tabgroup_close|" + sekme.groupId); });
    } else { // Sekme bir grupta değilse, yeni grup oluşturma ve var olan gruplara ekleme seçenekleri gösterilir
        satirOlustur("Yeni Grup Oluştur", function () { window.chrome.webview.postMessage("tabgroup_create|" + sekme.id); });
        (sonBilinenGruplar || []).forEach(function (grup) {
            satirOlustur("\"" + grup.name + "\" Grubuna Ekle", function () { window.chrome.webview.postMessage("tabgroup_add|" + sekme.id + "|" + grup.id); });
        });
    }

    sekmeGrupMenusu.style.left = ekranX + "px";
    sekmeGrupMenusu.style.top = ekranY + "px";
    sekmeGrupMenusu.classList.add("gorunur");
}

// Bu fonksiyon, native taraftan gelen yer imi listesini, en üst seviye öğeler ve (tek seviyeli) klasörler olarak çubuğa çizer
function yerImiCubugunuCiz(liste) {
    yerImiCubugu.innerHTML = ""; // Çubuk, her güncellemede tamamen yeniden çizilir, yer imi sayısı genelde az olduğu için bu basit yaklaşım yeterlidir

    if (!liste || liste.length === 0) { // Hiç yer imi yoksa
        const bosMesaj = document.createElement("div");
        bosMesaj.id = "yerimi-cubugu-bos-mesaj";
        bosMesaj.textContent = "Sık kullandığınız sayfaları yıldız butonuyla buraya ekleyebilirsiniz";
        yerImiCubugu.appendChild(bosMesaj);
        return;
    }

    const klasorler = new Map(); // Klasör adına göre gruplanan yer imleri, sırası ilk görüldükleri sıradır
    const kokOgeleri = []; // Klasörsüz (en üst seviye) yer imleri
    liste.forEach(function (kayit) {
        if (kayit.klasor) {
            if (!klasorler.has(kayit.klasor)) { klasorler.set(kayit.klasor, []); }
            klasorler.get(kayit.klasor).push(kayit);
        } else {
            kokOgeleri.push(kayit);
        }
    });

    // Bu fonksiyon, tek bir yer imi için tıklanabilir bir öğe oluşturur
    function yerImiOgesiOlustur(kayit) {
        const oge = document.createElement("div");
        oge.className = "yerimi-ogesi";
        oge.title = kayit.baslik || kayit.adres;

        const yerTutucu = document.createElement("div"); // Gerçek site ikonu bulunamazsa gösterilecek, vurgu renginden türetilen yedek kutu
        yerTutucu.className = "yerimi-favicon yerimi-favicon-yertutucu";

        let siteHostu = ""; // Google favicon servisine gönderilecek, yer iminin adresinden ayıklanan alan adı
        try { siteHostu = new URL(kayit.adres).hostname; } catch (hata) { siteHostu = ""; }

        if (siteHostu) { // Alan adı ayıklanabildiyse, yeni sekme sayfasındaki hızlı erişim kutularıyla aynı yöntemle gerçek site ikonu denenir
            const ikonOgesi = document.createElement("img");
            ikonOgesi.className = "yerimi-favicon";
            ikonOgesi.src = "https://www.google.com/s2/favicons?domain=" + encodeURIComponent(siteHostu) + "&sz=32";
            ikonOgesi.onerror = function () { ikonOgesi.replaceWith(yerTutucu); }; // İkon hiç bulunamazsa, yedek renkli kutu onun yerine konur
            oge.appendChild(ikonOgesi);
        } else {
            oge.appendChild(yerTutucu);
        }

        const baslik = document.createElement("span");
        baslik.textContent = kayit.baslik || kayit.adres;
        oge.appendChild(baslik);
        oge.addEventListener("click", function (olay) {
            olay.stopPropagation();
            window.chrome.webview.postMessage("openbookmarkbar|" + kayit.adres);
            document.querySelectorAll(".yerimi-klasor.acik").forEach(function (k) { k.classList.remove("acik"); }); // Bir öğeye tıklanınca açık klasör menüsü (varsa) kapanır
        });
        return oge;
    }

    kokOgeleri.forEach(function (kayit) { // Klasörsüz yer imleri, doğrudan çubuğa eklenir
        yerImiCubugu.appendChild(yerImiOgesiOlustur(kayit));
    });

    klasorler.forEach(function (kayitlar, klasorAdi) { // Her klasör, tıklanınca açılan bir açılır menü olarak eklenir
        const klasorOgesi = document.createElement("div");
        klasorOgesi.className = "yerimi-klasor";

        const klasorBasligi = document.createElement("div");
        klasorBasligi.className = "yerimi-ogesi";
        klasorBasligi.textContent = "📁 " + klasorAdi;
        klasorBasligi.addEventListener("click", function (olay) {
            olay.stopPropagation();
            const simdiAcikMi = klasorOgesi.classList.contains("acik");
            document.querySelectorAll(".yerimi-klasor.acik").forEach(function (k) { k.classList.remove("acik"); }); // Aynı anda sadece bir klasör menüsü açık olur
            if (!simdiAcikMi) { klasorOgesi.classList.add("acik"); }
        });

        const klasorMenu = document.createElement("div");
        klasorMenu.className = "yerimi-klasor-menu";
        kayitlar.forEach(function (kayit) { klasorMenu.appendChild(yerImiOgesiOlustur(kayit)); });

        klasorOgesi.appendChild(klasorBasligi);
        klasorOgesi.appendChild(klasorMenu);
        yerImiCubugu.appendChild(klasorOgesi);
    });
}

document.addEventListener("click", function () { // Çubuğun veya menünün dışında bir yere tıklanınca, açık klasör menüsü (varsa) kapanır
    document.querySelectorAll(".yerimi-klasor.acik").forEach(function (k) { k.classList.remove("acik"); });
});

// Native taraftan bir mesaj geldiğinde bu fonksiyon çalışır
window.chrome.webview.addEventListener("message", function (olay) {
    let veri; // Gelen mesajın JSON'dan çözülmüş halini tutacak değişken
    try {
        veri = JSON.parse(olay.data); // Native tarafın gönderdiği JSON metni, bir JavaScript nesnesine çevrilir
    } catch (hata) {
        return; // Eğer mesaj geçerli bir JSON değilse, güvenli bir şekilde yok sayılır
    }

    if (veri.type === "state") { // Eğer gelen mesaj, aktif sekmenin gezinme durumu güncellemesiyse
        // Hızlı erişim (yeni sekme) sayfasındayken, gerçek bir tarayıcıdaki gibi adres çubuğu boş gösterilir, kullanıcının gözüne sahte "tarayici.local" adresi batmaz
        const anaSayfadaMi = veri.url === "https://tarayici.local/newtab.html"; // Güncel adres, tam olarak hızlı erişim sayfasına mı ait kontrol edilir

        // Kullanıcı adres çubuğuna o an bir şey yazıyorsa, native taraftan gelen güncelleme onun yazdığını silmesin diye, sadece kutu seçili değilken adres güncellenir
        if (document.activeElement !== adresCubugu) {
            adresCubugu.value = anaSayfadaMi ? "" : veri.url; // Hızlı erişim sayfasındaysa boş, değilse güncel sayfa adresiyle değiştirilir
        }
        butonGeri.disabled = !veri.canGoBack; // Geri gidilemiyorsa, geri butonu pasif hale getirilir
        butonIleri.disabled = !veri.canGoForward; // İleri gidilemiyorsa, ileri butonu pasif hale getirilir
        butonYildiz.classList.toggle("yildiz-dolu", veri.bookmarked); // Sayfa yer imindeyse, yıldız butonu dolu (sarı) renkte gösterilir
        butonYildiz.setAttribute("aria-pressed", veri.bookmarked); // Ekran okuyucu da butonun basılı (aktif) durumunu duyursun diye

        butonKilit.classList.toggle("guvensiz", !veri.secure); // Bağlantı güvenli (https) değilse, kilit ikonu uyarı rengiyle gösterilir
        butonKilit.innerHTML = veri.secure ? "&#x1F512;" : "&#x26A0;"; // Güvenliyse kilit simgesi, değilse bir uyarı simgesi gösterilir
        butonKilit.title = veri.secure ? "Site bilgileri" : "Bağlantı güvenli değil, site bilgileri";
    }
    else if (veri.type === "tabs") { // Eğer gelen mesaj, güncel sekme listesiyse
        sonBilinenGruplar = veri.groups || []; // Güncel grup listesi saklanır, bağlam menüsünün "gruba ekle" seçenekleri bundan oluşturulur
        sekmeSeridiniCiz(veri.activeId, veri.tabs); // Sekme şeridi, gelen güncel bilgiyle güncellenir
    }
    else if (veri.type === "theme") { // Eğer gelen mesaj, güncel tema bilgisiyse
        temayiUygula(veri.theme); // Tema, arayüze anında uygulanır
    }
    else if (veri.type === "focusaddress") { // Eğer gelen mesaj, adres çubuğuna odaklanma isteğiyse (Ctrl+L kısayolundan gelir)
        adresCubugu.focus(); // Adres çubuğuna odaklanılır
        adresCubugu.select(); // Adres çubuğundaki mevcut metin tamamen seçilir, kullanıcı doğrudan yazmaya başlayabilir
    }
    else if (veri.type === "addresssuggestion_notconfirmed") { // Enter'a basıldığında native taraf, öneri panelinde klavyeyle seçili bir satır OLMADIĞINI bildiriyorsa, Enter normal adres gezinmesi olarak değerlendirilir
        window.chrome.webview.postMessage("close_addresssuggestions");
        window.chrome.webview.postMessage("navigate|" + enterBeklemedekiMetin);
    }
    else if (veri.type === "focustabsearch") { // Eğer gelen mesaj, sekme arama kutusuna odaklanma isteğiyse (Ctrl+Shift+A kısayolundan gelir)
        if (sekmeAramaGirdi.classList.contains("gizli")) { // Eğer arama kutusu şu anda kapalıysa
            sekmeAramaKutusunuAcKapat(); // Önce açılır
        } else { // Eğer zaten açıksa
            sekmeAramaGirdi.focus(); // Sadece odaklanılır
        }
    }
    else if (veri.type === "windowstate") { // Eğer gelen mesaj, pencerenin büyütülmüş olup olmadığı bilgisiyse
        butonBuyut.innerHTML = veri.maximized ? "&#x1F5D7;" : "&#x25A1;"; // Pencere büyütülmüşse "eski haline döndür" simgesi, değilse "büyüt" simgesi gösterilir
        butonBuyut.title = veri.maximized ? "Eski Haline Döndür" : "Büyüt"; // Butonun ipucu metni de duruma göre güncellenir
    }
    else if (veri.type === "bookmarkbar") { // Eğer gelen mesaj, güncel yer imi listesiyse
        yerImiCubugunuCiz(veri.entries); // Yer imi çubuğu, gelen güncel listeyle yeniden çizilir
    }
    else if (veri.type === "tabpreview") { // Eğer gelen mesaj, istenen bir sekmenin önizleme görüntüsüyse
        if (String(veri.id) !== onizlenenSekmeId) { // Cevap gelene kadar fare başka bir sekmeye geçmiş veya tamamen ayrılmış olabilir
            return; // Artık geçerli olmayan bu cevap sessizce yok sayılır
        }
        sekmeOnizlemeResmi.src = veri.image; // Görüntü, balonun içindeki <img> öğesine atanır
        sekmeOnizlemeBalonu.classList.add("gorunur"); // Balon görünür yapılır
    }
});

// Bu fonksiyon, native taraftan gelen tema bilgisini (vurgu rengi, sekme yoğunluğu, yazı boyutu, köşe yuvarlaklığı, açık/koyu mod) sayfaya uygular
function temayiUygula(tema) {
    document.documentElement.style.setProperty("--vurgu-rengi", tema.accent); // Vurgu rengi, CSS değişkenine atanır, aktif sekme çizgisi ve dolu yıldız gibi öğeler anında bu rengi kullanır

    const koseDegerleri = { kesik: "4px", orta: "10px", yuvarlak: "18px" }; // Üç kademeli köşe yuvarlaklığı seçeneğinin gerçek piksel karşılıkları
    const koseDegeri = koseDegerleri[tema.radius] || "10px"; // Seçilen kademeye karşılık gelen piksel değeri, tanınmayan bir değerse orta ölçü kullanılır
    document.documentElement.style.setProperty("--radius", koseDegeri); // Köşe yuvarlaklığı, CSS değişkenine atanır
    document.documentElement.style.setProperty("--radius-tab", koseDegeri); // Sekme köşe yuvarlaklığı da aynı değeri kullanır

    const yaziBoyutlari = { kucuk: "12px", orta: "14px", buyuk: "16px" }; // Üç kademeli yazı boyutu seçeneğinin gerçek piksel karşılıkları
    document.documentElement.style.setProperty("--yazi-boyutu", yaziBoyutlari[tema.fontSize] || "14px"); // Seçilen kademeye karşılık gelen piksel değeri uygulanır, tanınmayan bir değerse orta boy kullanılır

    document.body.classList.toggle("yogunluk-kompakt", tema.density === "kompakt"); // Sekme şeridinin kompakt görünüp görünmeyeceği, seçilen yoğunluğa göre ayarlanır

    if (tema.mode === "acik") { document.documentElement.setAttribute("data-tema", "acik"); } // Kullanıcı özellikle açık temayı seçtiyse, bu özellik eklenir
    else if (tema.mode === "yuksekkontrast") { document.documentElement.setAttribute("data-tema", "yuksekkontrast"); } // Erişilebilirlik için yüksek kontrast temayı seçtiyse, bu özellik eklenir
    else if (tema.mode === "koyu") { document.documentElement.removeAttribute("data-tema"); } // Koyu seçildiyse, özellik kaldırılır çünkü koyu zaten varsayılan temadır
    else { document.documentElement.removeAttribute("data-tema"); } // "Otomatik" seçildiyse de özellik kaldırılır, böylece işletim sisteminin tercihi (tokens.css'teki prefers-color-scheme kuralı) devreye girer

    aktifSekmeGostergesiniGuncelle(sonBilinenAktifId); // Köşe yuvarlaklığı değişimi sekme genişliklerini etkileyebileceği için, gösterge çizgisinin konumu yeniden hesaplanır

    yerImiCubugu.classList.toggle("gizli", tema.showBookmarkBar === false); // Yer imi çubuğu, Ayarlar'daki tercihe göre gösterilir/gizlenir

    document.body.classList.toggle("enerji-tasarrufu", tema.energySaver === true); // Enerji tasarrufu modu açıkken, chrome.css'teki kural geçiş animasyonlarını kısar
}

window.chrome.webview.postMessage("get_theme"); // Sayfa yüklenir yüklenmez, native taraftan kayıtlı tema ayarları istenir, böylece program her açıldığında kullanıcının seçtiği tema uygulanır
window.chrome.webview.postMessage("get_bookmarkbar"); // Sayfa yüklenir yüklenmez, native taraftan kayıtlı yer imleri de istenir, böylece çubuk programın her açılışında doğru doldurulur
// Program ilk açıldığında native taraf, toolbar sayfası henüz yüklenmeden ilk sekmeyi oluşturup sekme listesini göndermeye çalışıyordu, bu mesaj kayboluyor ve ilk sekme hiç çizilmiyordu (sadece "+" butonu görünüyordu), ikinci sekme açılınca her ikisi de doğru çiziliyordu
// Bu yüzden toolbar sayfası kendisi yüklendiğinde, güncel sekme listesini native taraftan AYRICA isteyerek bu kaybı telafi eder
window.chrome.webview.postMessage("get_tabs");

window.addEventListener("resize", function () { // Pencere boyutu değiştiğinde
    sekmeGenislikleriniAyarla(sonBilinenSekmeler); // Kullanılabilir alan değiştiği için, sekme genişlikleri yeniden hesaplanır
    aktifSekmeGostergesiniGuncelle(sonBilinenAktifId); // Gösterge çizgisinin konumu, yeni genişliklere göre yeniden hesaplanır
});
