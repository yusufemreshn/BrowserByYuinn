// Bu dosya, Settings.h içinde tanımı yapılan ayar okuma ve kaydetme fonksiyonlarının gerçek çalışma mantığını içerir.
// Ayarlar, kullanıcının AppData\Roaming\tarayici klasöründeki settings.json dosyasında, tek bir JSON nesnesi olarak (dizi değil) tutulur.

#include "Settings.h" // Bu dosyanın, hangi fonksiyonların gövdesini yazdığını bilmesi için kendi başlık dosyası eklenir
#include "JsonYardimci.h" // Metinleri JSON içine yazma ve JSON nesnesinden alan okuma yardımcıları için eklenir
#include "VeriYolu.h" // Verilerin saklanacağı ana klasörün yolunu bulan ortak fonksiyon için eklenir
#include "Utf8Yardimci.h" // Dosyaları UTF-8 kodlamasıyla okuyup yazan ortak yardımcı fonksiyonlar için eklenir, Türkçe karakterlerin doğru saklanmasını sağlar

namespace Ayarlar // Tüm ayar fonksiyonları, başka isimlerle çakışmaması için bu isim alanının içine konur
{
    // Bu fonksiyon, settings.json dosyasının tam yolunu döndürür
    static std::wstring AyarDosyaYolu()
    {
        return VeriKlasoruAl() + L"\\settings.json"; // Ana veri klasörünün altındaki settings.json dosyasının yolu döndürülür
    }

    // Bu fonksiyon, diskteki settings.json dosyasını okuyup bir AyarVerisi nesnesi olarak döndürür, dosya yoksa veya bozuksa varsayılan ayarları döndürür
    AyarVerisi Oku()
    {
        AyarVerisi ayarlar; // Önce tüm alanları varsayılan değerleriyle dolu bir nesne oluşturulur, dosyada eksik olan alanlar böylece varsayılanında kalır

        std::wstring icerik = Utf8DosyaOku(AyarDosyaYolu()); // settings.json dosyasının tüm içeriği, UTF-8 kodlaması çözülerek okunur
        if (icerik.empty()) // Eğer dosya henüz hiç oluşturulmadıysa veya tamamen boşsa
        {
            return ayarlar; // Varsayılan ayarlar olduğu gibi döndürülür
        }

        // Dosyada bulunan her alan, varsa varsayılan değerin üzerine yazılır, bulunamayan alanlar varsayılanında kalır
        std::wstring okunanAramaMotoru = JsonAlaniBul(icerik, L"aramaMotoru");
        if (!okunanAramaMotoru.empty()) { ayarlar.aramaMotoru = okunanAramaMotoru; }

        std::wstring okunanAnaSayfa = JsonAlaniBul(icerik, L"anaSayfa");
        if (!okunanAnaSayfa.empty()) { ayarlar.anaSayfa = okunanAnaSayfa; }

        std::wstring okunanVurguRengi = JsonAlaniBul(icerik, L"vurguRengi");
        if (!okunanVurguRengi.empty()) { ayarlar.vurguRengi = okunanVurguRengi; }

        std::wstring okunanSekmeYogunlugu = JsonAlaniBul(icerik, L"sekmeYogunlugu");
        if (!okunanSekmeYogunlugu.empty()) { ayarlar.sekmeYogunlugu = okunanSekmeYogunlugu; }

        std::wstring okunanYaziBoyutu = JsonAlaniBul(icerik, L"yaziBoyutu");
        if (!okunanYaziBoyutu.empty()) { ayarlar.yaziBoyutu = okunanYaziBoyutu; }

        std::wstring okunanTemaModu = JsonAlaniBul(icerik, L"temaModu");
        if (!okunanTemaModu.empty()) { ayarlar.temaModu = okunanTemaModu; }

        std::wstring okunanKoseYuvarlakligi = JsonAlaniBul(icerik, L"koseYuvarlakligi");
        if (!okunanKoseYuvarlakligi.empty()) { ayarlar.koseYuvarlakligi = okunanKoseYuvarlakligi; }

        std::wstring okunanIlkAcilis = JsonAlaniBul(icerik, L"ilkAcilisTamamlandiMi"); // Bu alan da bir metin ("evet"/"hayir") olarak saklanır, diğer tüm alanlarla aynı basit biçimi korumak için
        ayarlar.ilkAcilisTamamlandiMi = (okunanIlkAcilis == L"evet"); // Metin "evet" ise doğru, aksi halde (boşsa veya başka bir şeyse) yanlış kabul edilir

        ayarlar.onaylananRizaSurumu = JsonAlaniBul(icerik, L"onaylananRizaSurumu"); // Alan dosyada hiç yoksa (eski bir ayar dosyasıysa) boş kalır, bu da çalışan .exe'nin güncel sürümüyle asla eşleşmeyeceği için onay ekranının gösterilmesini garantiler

        std::wstring okunanYerImiCubugu = JsonAlaniBul(icerik, L"yerImiCubuguGoster");
        if (!okunanYerImiCubugu.empty()) { ayarlar.yerImiCubuguGoster = (okunanYerImiCubugu == L"evet"); } // Alan dosyada hiç yoksa (eski bir ayar dosyasıysa), varsayılan (açık) değerinde kalır

        std::wstring okunanKilitlenmeDakikasi = JsonAlaniBul(icerik, L"sifreKasasiKilitlenmeDakikasi");
        if (!okunanKilitlenmeDakikasi.empty()) { ayarlar.sifreKasasiKilitlenmeDakikasi = _wtoi(okunanKilitlenmeDakikasi.c_str()); } // Alan dosyada hiç yoksa, varsayılan (5 dakika) değerinde kalır

        ayarlar.yeniSekmeArkaPlanYolu = JsonAlaniBul(icerik, L"yeniSekmeArkaPlanYolu"); // Alan dosyada hiç yoksa, boş (varsayılan desenli arka plan) kalır

        std::wstring okunanUykuEsigi = JsonAlaniBul(icerik, L"uykuSekmeEsigiDakika");
        if (!okunanUykuEsigi.empty()) { ayarlar.uykuSekmeEsigiDakika = _wtoi(okunanUykuEsigi.c_str()); } // Alan dosyada hiç yoksa, varsayılan (2 dakika) değerinde kalır

        std::wstring okunanCikistaTemizle = JsonAlaniBul(icerik, L"cikistaTemizle");
        if (!okunanCikistaTemizle.empty()) { ayarlar.cikistaTemizle = (okunanCikistaTemizle == L"evet"); }

        ayarlar.cikistaTemizleKategoriler = JsonAlaniBul(icerik, L"cikistaTemizleKategoriler"); // Alan dosyada hiç yoksa, boş (hiçbir kategori seçili değil) kalır

        ayarlar.haricTutulanSiteler = JsonAlaniBul(icerik, L"haricTutulanSiteler"); // Alan dosyada hiç yoksa, boş (hiç hariç tutulan site yok) kalır

        std::wstring okunanIndirmeKonumuSor = JsonAlaniBul(icerik, L"indirmeKonumunuHerSeferSor");
        if (!okunanIndirmeKonumuSor.empty()) { ayarlar.indirmeKonumunuHerSeferSor = (okunanIndirmeKonumuSor == L"evet"); }

        std::wstring okunanEnerjiTasarrufu = JsonAlaniBul(icerik, L"enerjiTasarrufuModu");
        if (!okunanEnerjiTasarrufu.empty()) { ayarlar.enerjiTasarrufuModu = (okunanEnerjiTasarrufu == L"evet"); }

        std::wstring okunanOturumGeriYukle = JsonAlaniBul(icerik, L"sonOturumuGeriYukle");
        if (!okunanOturumGeriYukle.empty()) { ayarlar.sonOturumuGeriYukle = (okunanOturumGeriYukle == L"evet"); }

        std::wstring okunanSizintiKontrolu = JsonAlaniBul(icerik, L"sizintiKontroluAcikMi");
        if (!okunanSizintiKontrolu.empty()) { ayarlar.sizintiKontroluAcikMi = (okunanSizintiKontrolu == L"evet"); }

        std::wstring okunanReklamEngelleme = JsonAlaniBul(icerik, L"reklamEngellemeAcikMi");
        if (!okunanReklamEngelleme.empty()) { ayarlar.reklamEngellemeAcikMi = (okunanReklamEngelleme == L"evet"); }

        std::wstring okunanIzleyiciSeviyesi = JsonAlaniBul(icerik, L"izleyiciEngellemeSeviyesi");
        if (!okunanIzleyiciSeviyesi.empty()) { ayarlar.izleyiciEngellemeSeviyesi = okunanIzleyiciSeviyesi; }

        std::wstring okunanGuvenliGezinme = JsonAlaniBul(icerik, L"guvenliGezinmeAcikMi");
        if (!okunanGuvenliGezinme.empty()) { ayarlar.guvenliGezinmeAcikMi = (okunanGuvenliGezinme == L"evet"); }

        std::wstring okunanDil = JsonAlaniBul(icerik, L"tarayiciDili");
        if (!okunanDil.empty()) { ayarlar.tarayiciDili = okunanDil; }

        // Özelleştirilebilir kısayollar, her biri tek karakterlik bir metin olarak saklanır, dosyada yoksa (veya boşsa) varsayılan harfinde kalır
        auto kisayolOku = [&icerik](const wchar_t* alanAdi, wchar_t& hedef)
        {
            std::wstring okunan = JsonAlaniBul(icerik, alanAdi);
            if (!okunan.empty()) { hedef = okunan[0]; }
        };
        kisayolOku(L"kisayolYeniSekme", ayarlar.kisayolYeniSekme);
        kisayolOku(L"kisayolSekmeKapat", ayarlar.kisayolSekmeKapat);
        kisayolOku(L"kisayolCogalt", ayarlar.kisayolCogalt);
        kisayolOku(L"kisayolYazdir", ayarlar.kisayolYazdir);
        kisayolOku(L"kisayolSayfadaBul", ayarlar.kisayolSayfadaBul);
        kisayolOku(L"kisayolYenile", ayarlar.kisayolYenile);
        kisayolOku(L"kisayolAdres", ayarlar.kisayolAdres);

        return ayarlar; // Dosyadan okunan (veya bulunamayan alanlar için varsayılan kalan) ayarlar döndürülür
    }

    // Bu fonksiyon, verilen ayarları settings.json dosyasına kaydeder
    void Kaydet(const AyarVerisi& ayarlar)
    {
        // Tüm ayar alanları, tek bir JSON nesnesi metnine birleştirilir
        std::wstring icerik = L"{\"aramaMotoru\":\"" + JsonIcinKacisla(ayarlar.aramaMotoru)
            + L"\",\"anaSayfa\":\"" + JsonIcinKacisla(ayarlar.anaSayfa)
            + L"\",\"vurguRengi\":\"" + JsonIcinKacisla(ayarlar.vurguRengi)
            + L"\",\"sekmeYogunlugu\":\"" + JsonIcinKacisla(ayarlar.sekmeYogunlugu)
            + L"\",\"yaziBoyutu\":\"" + JsonIcinKacisla(ayarlar.yaziBoyutu)
            + L"\",\"temaModu\":\"" + JsonIcinKacisla(ayarlar.temaModu)
            + L"\",\"koseYuvarlakligi\":\"" + JsonIcinKacisla(ayarlar.koseYuvarlakligi)
            + L"\",\"ilkAcilisTamamlandiMi\":\"" + (ayarlar.ilkAcilisTamamlandiMi ? L"evet" : L"hayir")
            + L"\",\"onaylananRizaSurumu\":\"" + JsonIcinKacisla(ayarlar.onaylananRizaSurumu)
            + L"\",\"yerImiCubuguGoster\":\"" + (ayarlar.yerImiCubuguGoster ? L"evet" : L"hayir")
            + L"\",\"sifreKasasiKilitlenmeDakikasi\":\"" + std::to_wstring(ayarlar.sifreKasasiKilitlenmeDakikasi)
            + L"\",\"yeniSekmeArkaPlanYolu\":\"" + JsonIcinKacisla(ayarlar.yeniSekmeArkaPlanYolu)
            + L"\",\"uykuSekmeEsigiDakika\":\"" + std::to_wstring(ayarlar.uykuSekmeEsigiDakika)
            + L"\",\"cikistaTemizle\":\"" + (ayarlar.cikistaTemizle ? L"evet" : L"hayir")
            + L"\",\"cikistaTemizleKategoriler\":\"" + JsonIcinKacisla(ayarlar.cikistaTemizleKategoriler)
            + L"\",\"haricTutulanSiteler\":\"" + JsonIcinKacisla(ayarlar.haricTutulanSiteler)
            + L"\",\"indirmeKonumunuHerSeferSor\":\"" + (ayarlar.indirmeKonumunuHerSeferSor ? L"evet" : L"hayir")
            + L"\",\"enerjiTasarrufuModu\":\"" + (ayarlar.enerjiTasarrufuModu ? L"evet" : L"hayir")
            + L"\",\"sonOturumuGeriYukle\":\"" + (ayarlar.sonOturumuGeriYukle ? L"evet" : L"hayir")
            + L"\",\"sizintiKontroluAcikMi\":\"" + (ayarlar.sizintiKontroluAcikMi ? L"evet" : L"hayir")
            + L"\",\"reklamEngellemeAcikMi\":\"" + (ayarlar.reklamEngellemeAcikMi ? L"evet" : L"hayir")
            + L"\",\"izleyiciEngellemeSeviyesi\":\"" + JsonIcinKacisla(ayarlar.izleyiciEngellemeSeviyesi)
            + L"\",\"guvenliGezinmeAcikMi\":\"" + (ayarlar.guvenliGezinmeAcikMi ? L"evet" : L"hayir")
            + L"\",\"tarayiciDili\":\"" + JsonIcinKacisla(ayarlar.tarayiciDili)
            + L"\",\"kisayolYeniSekme\":\"" + ayarlar.kisayolYeniSekme
            + L"\",\"kisayolSekmeKapat\":\"" + ayarlar.kisayolSekmeKapat
            + L"\",\"kisayolCogalt\":\"" + ayarlar.kisayolCogalt
            + L"\",\"kisayolYazdir\":\"" + ayarlar.kisayolYazdir
            + L"\",\"kisayolSayfadaBul\":\"" + ayarlar.kisayolSayfadaBul
            + L"\",\"kisayolYenile\":\"" + ayarlar.kisayolYenile
            + L"\",\"kisayolAdres\":\"" + ayarlar.kisayolAdres
            + L"\"}";
        Utf8DosyaYaz(AyarDosyaYolu(), icerik); // Hazırlanan JSON metni, UTF-8 kodlamasıyla dosyaya yazılır
    }
}
