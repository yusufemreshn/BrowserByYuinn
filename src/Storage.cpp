// Bu dosya, Storage.h içinde tanımı yapılan geçmiş ve yer imi fonksiyonlarının gerçek çalışma mantığını içerir.
// Burada, verilerin diske nasıl bir metin (JSON benzeri) biçiminde yazıldığı, ve bu metnin diskten okunurken nasıl tekrar listeye çevrildiği yazılıdır.
// Bu dosya, dışarıdan hazır bir JSON kütüphanesi kullanmaz, çünkü bizim dosya biçimimiz hep kendi yazdığımız basit ve sabit bir biçimdedir, bu yüzden JsonYardimci.h'deki ortak küçük okuma/yazma yardımcıları yeterlidir.

#include "Storage.h" // Bu dosyanın, hangi fonksiyonların gövdesini yazdığını bilmesi için kendi başlık dosyası eklenir
#include "JsonYardimci.h" // Metinleri JSON içine yazma, JSON'dan çözme ve JSON nesnesinden alan okuma yardımcıları için eklenir
#include "VeriYolu.h" // Verilerin saklanacağı ana klasörün yolunu bulan ortak fonksiyon için eklenir
#include "Utf8Yardimci.h" // Dosyaları UTF-8 kodlamasıyla okuyup yazan ortak yardımcı fonksiyonlar için eklenir, Türkçe karakterlerin doğru saklanmasını sağlar
#include <windows.h> // Zaman bilgisini okumak için Windows fonksiyonlarını kullanmak için eklenir
#include <algorithm> // Yer imlerini yeniden sıralarken, gelen listenin gerçek bir permütasyon olup olmadığını doğrulamak için std::sort kullanmak üzere eklenir

namespace Depolama // BrowserWindow.cpp dosyasındaki isim alanıyla aynı isim alanı burada da kullanılır, çünkü aynı fonksiyonların gövdeleri yazılıyor
{
    // Bu fonksiyon, bir kayıt listesini, dosyaya yazılabilecek bir JSON dizisi metnine çevirir
    static std::wstring ListeyiJsonYap(const std::vector<SayfaKaydi>& liste)
    {
        std::wstring sonuc = L"["; // JSON dizisinin açılış köşeli parantezi ile başlanır
        for (size_t i = 0; i < liste.size(); i++) // Listedeki her kayıt sırayla gezilir
        {
            if (i > 0) // İlk kayıttan sonraki her kayıttan önce
            {
                sonuc += L","; // Kayıtları birbirinden ayırmak için virgül eklenir
            }
            // Her kayıt, adres, başlık, zaman ve klasör alanlarını içeren bir JSON nesnesi olarak yazılır, her metin alanı kaçışlanarak eklenir
            sonuc += L"{\"adres\":\"" + JsonIcinKacisla(liste[i].adres) + L"\",\"baslik\":\"" + JsonIcinKacisla(liste[i].baslik) + L"\",\"zaman\":\"" + JsonIcinKacisla(liste[i].zaman) + L"\",\"klasor\":\"" + JsonIcinKacisla(liste[i].klasor) + L"\"}";
        }
        sonuc += L"]"; // JSON dizisinin kapanış köşeli parantezi eklenir
        return sonuc; // Hazırlanan JSON dizisi metni döndürülür
    }

    // Bu fonksiyon, dosyadan okunan bir JSON dizisi metnini, bir kayıt listesine çevirir, bizim kendi yazdığımız sabit biçime göre çalışır, başka bir biçimi anlamaz
    static std::vector<SayfaKaydi> JsonDiziyiListeYap(const std::wstring& jsonMetni)
    {
        std::vector<SayfaKaydi> liste; // Sonuç olarak döndürülecek boş bir liste oluşturulur

        size_t konum = 0; // Metin içinde ilerlerken kullanılacak konum sayacı
        while (true) // Metindeki tüm nesneler bulunana kadar devam eden bir döngü
        {
            size_t nesneBaslangici = jsonMetni.find(L'{', konum); // Bir sonraki JSON nesnesinin açılış süslü parantezi aranır
            if (nesneBaslangici == std::wstring::npos) // Eğer daha fazla nesne bulunamadıysa
            {
                break; // Döngüden çıkılır, tüm nesneler işlendi demektir
            }
            size_t nesneBitisi = jsonMetni.find(L'}', nesneBaslangici); // Bu nesnenin kapanış süslü parantezi aranır
            if (nesneBitisi == std::wstring::npos) // Eğer kapanış bulunamadıysa, dosya bozuk demektir
            {
                break; // Döngüden çıkılır, elde bozuk veri işlenmez
            }

            std::wstring nesneMetni = jsonMetni.substr(nesneBaslangici, nesneBitisi - nesneBaslangici + 1); // Bu tek nesnenin tam metni ayıklanır

            SayfaKaydi kayit; // Bu nesneden oluşturulacak yeni bir kayıt
            kayit.adres = JsonAlaniBul(nesneMetni, L"adres"); // Nesnenin adres alanı okunur
            kayit.baslik = JsonAlaniBul(nesneMetni, L"baslik"); // Nesnenin başlık alanı okunur
            kayit.zaman = JsonAlaniBul(nesneMetni, L"zaman"); // Nesnenin zaman alanı okunur
            kayit.klasor = JsonAlaniBul(nesneMetni, L"klasor"); // Nesnenin klasör alanı okunur, eski (bu alan eklenmeden önce yazılmış) kayıtlarda bu alan yoktur, bu durumda boş metin döner, bu da "klasörsüz" demektir

            if (!kayit.adres.empty()) // Eğer bu nesneden geçerli bir adres okunabildiyse, yani nesne gerçekten bir kayıt gibi görünüyorsa
            {
                liste.push_back(kayit); // Kayıt, sonuç listesine eklenir
            }

            konum = nesneBitisi + 1; // Arama konumu, bu nesnenin bittiği yerin bir ötesine taşınır, böylece bir sonraki nesne aranabilir
        }

        return liste; // Dosyadan okunup listeye çevrilmiş kayıtlar döndürülür
    }

    // Bu fonksiyon, verilen dosya yolundaki bir JSON dosyasını okuyup bir kayıt listesine çevirir, dosya yoksa veya bozuksa boş bir liste döndürür, asla programı çökertmez
    static std::vector<SayfaKaydi> DosyadanOku(const std::wstring& dosyaYolu)
    {
        std::wstring icerik = Utf8DosyaOku(dosyaYolu); // Dosyanın tüm içeriği, UTF-8 kodlaması çözülerek bir metin olarak okunur
        return JsonDiziyiListeYap(icerik); // Okunan metin, JsonDiziyiListeYap fonksiyonuyla bir kayıt listesine çevrilip döndürülür, bu fonksiyon bozuk veriyi de tolere eder
    }

    // Bu fonksiyon, verilen bir kayıt listesini, verilen dosya yoluna JSON biçiminde yazar
    static void DosyayaYaz(const std::wstring& dosyaYolu, const std::vector<SayfaKaydi>& liste)
    {
        Utf8DosyaYaz(dosyaYolu, ListeyiJsonYap(liste)); // Liste, JSON metnine çevrilip, UTF-8 kodlamasıyla dosyaya yazılır
    }

    // Bu fonksiyon, şu anki tarih ve saati, ISO biçiminde bir metin olarak döndürür, kayıtların zaman damgası olarak kullanılır
    static std::wstring SuankiZamaniAl()
    {
        SYSTEMTIME zaman; // Şu anki zamanı tutacak Windows yapısı
        GetSystemTime(&zaman); // Sistemin şu anki evrensel saati bu yapıya okunur
        wchar_t tamponMetin[32]; // Zamanın metin haline yazılacağı tampon
        swprintf_s(tamponMetin, L"%04d-%02d-%02dT%02d:%02d:%02dZ", zaman.wYear, zaman.wMonth, zaman.wDay, zaman.wHour, zaman.wMinute, zaman.wSecond); // Yıl, ay, gün, saat, dakika, saniye bilgileri ISO biçiminde birleştirilir
        return tamponMetin; // Hazırlanan zaman metni döndürülür
    }

    // Bu fonksiyon, geçmiş dosyasının tam yolunu döndürür
    static std::wstring GecmisDosyaYolu()
    {
        return VeriKlasoruAl() + L"\\history.json"; // Veri klasörünün altındaki history.json dosyasının yolu döndürülür
    }

    // Bu fonksiyon, yer imleri dosyasının tam yolunu döndürür
    static std::wstring YerImleriDosyaYolu()
    {
        return VeriKlasoruAl() + L"\\bookmarks.json"; // Veri klasörünün altındaki bookmarks.json dosyasının yolu döndürülür
    }

    // Bu fonksiyon, verilen bir web sayfasını geçmiş listesinin sonuna ekler
    void GecmiseEkle(const std::wstring& adres, const std::wstring& baslik)
    {
        if (adres.rfind(L"https://tarayici.local", 0) == 0) // Eğer ziyaret edilen adres, programın kendi dahili sayfalarından (toolbar veya ayarlar) birine aitse
        {
            return; // Bu, kullanıcının gerçekten ziyaret ettiği bir sayfa olmadığı için geçmişe eklenmez
        }

        std::vector<SayfaKaydi> liste = DosyadanOku(GecmisDosyaYolu()); // Mevcut geçmiş listesi diskten okunur

        SayfaKaydi yeniKayit; // Eklenecek yeni kayıt hazırlanır
        yeniKayit.adres = adres; // Ziyaret edilen adres atanır
        yeniKayit.baslik = baslik; // Sayfanın başlığı atanır
        yeniKayit.zaman = SuankiZamaniAl(); // Şu anki zaman, ziyaret zamanı olarak atanır

        liste.push_back(yeniKayit); // Yeni kayıt, listenin sonuna eklenir

        const size_t azamiKayitSayisi = 5000; // Geçmiş dosyasının çok büyümemesi için üst sınır belirlenir
        if (liste.size() > azamiKayitSayisi) // Eğer liste bu sınırı aştıysa
        {
            liste.erase(liste.begin(), liste.begin() + (liste.size() - azamiKayitSayisi)); // En eski kayıtlar, liste sınırın içine dönene kadar baştan silinir
        }

        DosyayaYaz(GecmisDosyaYolu(), liste); // Güncellenmiş liste, tekrar diske yazılır
    }

    // Bu fonksiyon, verilen bir web sayfasını yer imlerine ekler, eğer zaten ekliyse tekrar eklemez
    void YerImiEkle(const std::wstring& adres, const std::wstring& baslik, const std::wstring& klasor)
    {
        std::vector<SayfaKaydi> liste = DosyadanOku(YerImleriDosyaYolu()); // Mevcut yer imi listesi diskten okunur

        for (const SayfaKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            if (kayit.adres == adres) // Eğer bu adres zaten yer imlerinde varsa
            {
                return; // Tekrar eklemeye gerek olmadığı için fonksiyondan çıkılır
            }
        }

        SayfaKaydi yeniKayit; // Eklenecek yeni kayıt hazırlanır
        yeniKayit.adres = adres; // Yer imine eklenen adres atanır
        yeniKayit.baslik = baslik; // Sayfanın başlığı atanır
        yeniKayit.zaman = SuankiZamaniAl(); // Şu anki zaman, eklenme zamanı olarak atanır
        yeniKayit.klasor = klasor; // Yer iminin ait olduğu klasör atanır, boşsa klasörsüz demektir

        liste.push_back(yeniKayit); // Yeni kayıt, listenin sonuna eklenir
        DosyayaYaz(YerImleriDosyaYolu(), liste); // Güncellenmiş liste, tekrar diske yazılır
    }

    // Bu fonksiyon, verilen adrese sahip yer iminin klasörünü değiştirir
    void YerImiKlasoruDegistir(const std::wstring& adres, const std::wstring& yeniKlasor)
    {
        std::vector<SayfaKaydi> liste = DosyadanOku(YerImleriDosyaYolu()); // Mevcut yer imi listesi diskten okunur
        for (SayfaKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            if (kayit.adres == adres) // Eğer bu, taşınmak istenen yer imiyse
            {
                kayit.klasor = yeniKlasor; // Klasörü güncellenir
                DosyayaYaz(YerImleriDosyaYolu(), liste); // Güncellenmiş liste diske yazılır
                return; // İşlem tamamlandı
            }
        }
    }

    // Bu fonksiyon, yer imlerinin tamamını, verilen yeni adres sırasına göre yeniden diziler
    void YerImleriniSirala(const std::vector<std::wstring>& yeniSiraAdresleri)
    {
        std::vector<SayfaKaydi> liste = DosyadanOku(YerImleriDosyaYolu()); // Mevcut yer imi listesi diskten okunur
        if (yeniSiraAdresleri.size() != liste.size()) // Gelen liste, mevcut yer imi sayısıyla eşleşmiyorsa (bayat bir istekse) güvenilmez kabul edilir
        {
            return;
        }

        std::vector<std::wstring> mevcutAdreslerSirali; // Karşılaştırma için mevcut adresler sıralanmış halde toplanır
        for (const SayfaKaydi& kayit : liste) { mevcutAdreslerSirali.push_back(kayit.adres); }
        std::vector<std::wstring> yeniAdreslerSirali = yeniSiraAdresleri;
        std::sort(mevcutAdreslerSirali.begin(), mevcutAdreslerSirali.end());
        std::sort(yeniAdreslerSirali.begin(), yeniAdreslerSirali.end());
        if (mevcutAdreslerSirali != yeniAdreslerSirali) // Gelen liste, gerçekten var olan yer imlerinin bir permütasyonu değilse güvenilmez kabul edilir
        {
            return;
        }

        std::vector<SayfaKaydi> yeniListe; // Yeni sıraya göre yeniden dizilecek liste
        yeniListe.reserve(liste.size());
        for (const std::wstring& adres : yeniSiraAdresleri) // Gelen yeni sıradaki her adres için
        {
            for (const SayfaKaydi& kayit : liste) // Mevcut listede bu adrese sahip kayıt aranır
            {
                if (kayit.adres == adres)
                {
                    yeniListe.push_back(kayit);
                    break;
                }
            }
        }
        DosyayaYaz(YerImleriDosyaYolu(), yeniListe); // Yeni sıradaki liste, diske yazılır
    }

    // Bu fonksiyon, verilen bir web sayfasını yer imlerinden kaldırır
    void YerImiKaldir(const std::wstring& adres)
    {
        std::vector<SayfaKaydi> liste = DosyadanOku(YerImleriDosyaYolu()); // Mevcut yer imi listesi diskten okunur

        std::vector<SayfaKaydi> yeniListe; // Silinecek kayıt hariç tutularak yeniden oluşturulacak liste
        for (const SayfaKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            if (kayit.adres != adres) // Eğer bu kayıt, silinmek istenen adresle eşleşmiyorsa
            {
                yeniListe.push_back(kayit); // Kayıt, yeni listeye aktarılır
            }
        }

        DosyayaYaz(YerImleriDosyaYolu(), yeniListe); // Silinen kayıt hariç yeni liste, diske yazılır
    }

    // Bu fonksiyon, verilen bir web sayfasının şu anda yer imlerinde olup olmadığını kontrol eder
    bool YerImindeMi(const std::wstring& adres)
    {
        std::vector<SayfaKaydi> liste = DosyadanOku(YerImleriDosyaYolu()); // Mevcut yer imi listesi diskten okunur
        for (const SayfaKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            if (kayit.adres == adres) // Eğer bu adres yer imlerinde bulunduysa
            {
                return true; // Doğru değeri döndürülür
            }
        }
        return false; // Adres hiçbir kayıtta bulunamadıysa yanlış değeri döndürülür
    }

    // Bu fonksiyon, yer imlerindeki tüm kayıtları bir liste olarak döndürür
    std::vector<SayfaKaydi> YerImleriniOku()
    {
        return DosyadanOku(YerImleriDosyaYolu()); // Yer imi dosyası okunup, içindeki liste olduğu gibi döndürülür
    }

    // Bu fonksiyon, okuma listesi dosyasının tam yolunu döndürür
    static std::wstring OkumaListesiDosyaYolu()
    {
        return VeriKlasoruAl() + L"\\reading-list.json"; // Veri klasörünün altındaki reading-list.json dosyasının yolu döndürülür
    }

    // Bu fonksiyon, verilen bir web sayfasını okuma listesine ekler, eğer zaten ekliyse tekrar eklemez
    void OkumaListesineEkle(const std::wstring& adres, const std::wstring& baslik)
    {
        std::vector<SayfaKaydi> liste = DosyadanOku(OkumaListesiDosyaYolu()); // Mevcut okuma listesi diskten okunur
        for (const SayfaKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            if (kayit.adres == adres) // Eğer bu adres zaten okuma listesinde varsa
            {
                return; // Tekrar eklemeye gerek olmadığı için fonksiyondan çıkılır
            }
        }

        SayfaKaydi yeniKayit; // Eklenecek yeni kayıt hazırlanır
        yeniKayit.adres = adres;
        yeniKayit.baslik = baslik;
        yeniKayit.zaman = SuankiZamaniAl(); // Şu anki zaman, eklenme zamanı olarak atanır

        liste.push_back(yeniKayit); // Yeni kayıt, listenin sonuna eklenir
        DosyayaYaz(OkumaListesiDosyaYolu(), liste); // Güncellenmiş liste, tekrar diske yazılır
    }

    // Bu fonksiyon, verilen bir web sayfasını okuma listesinden kaldırır
    void OkumaListesindenKaldir(const std::wstring& adres)
    {
        std::vector<SayfaKaydi> liste = DosyadanOku(OkumaListesiDosyaYolu()); // Mevcut okuma listesi diskten okunur
        std::vector<SayfaKaydi> yeniListe; // Silinecek kayıt hariç tutularak yeniden oluşturulacak liste
        for (const SayfaKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            if (kayit.adres != adres) // Eğer bu kayıt, silinmek istenen adresle eşleşmiyorsa
            {
                yeniListe.push_back(kayit); // Kayıt, yeni listeye aktarılır
            }
        }
        DosyayaYaz(OkumaListesiDosyaYolu(), yeniListe); // Silinen kayıt hariç yeni liste, diske yazılır
    }

    // Bu fonksiyon, verilen bir web sayfasının şu anda okuma listesinde olup olmadığını kontrol eder
    bool OkumaListesindeMi(const std::wstring& adres)
    {
        std::vector<SayfaKaydi> liste = DosyadanOku(OkumaListesiDosyaYolu()); // Mevcut okuma listesi diskten okunur
        for (const SayfaKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            if (kayit.adres == adres) // Eğer bu adres okuma listesinde bulunduysa
            {
                return true;
            }
        }
        return false;
    }

    // Bu fonksiyon, okuma listesindeki tüm kayıtları bir liste olarak döndürür
    std::vector<SayfaKaydi> OkumaListesiniOku()
    {
        return DosyadanOku(OkumaListesiDosyaYolu()); // Okuma listesi dosyası okunup, içindeki liste olduğu gibi döndürülür
    }

    // Bu fonksiyon, geçmişteki tüm kayıtları bir liste olarak döndürür
    std::vector<SayfaKaydi> GecmisiOku()
    {
        return DosyadanOku(GecmisDosyaYolu()); // Geçmiş dosyası okunup, içindeki liste olduğu gibi döndürülür
    }

    // Bu fonksiyon, geçmişteki kayıtları siler, Ayarlar sayfasındaki "Geçmişi Temizle" işlevi ve zaman aralığı seçilebilen veri temizleme işlevi tarafından kullanılır
    void GecmisiTemizle(const std::wstring& baslangicZamaniIso)
    {
        if (baslangicZamaniIso.empty()) // Bir başlangıç zamanı verilmediyse, tüm geçmiş istenmiş demektir
        {
            DosyayaYaz(GecmisDosyaYolu(), {}); // Geçmiş dosyasının üzerine boş bir liste yazılır, böylece dosya boşalmış olur
            return; // İşlem tamamlandı
        }

        std::vector<SayfaKaydi> liste = DosyadanOku(GecmisDosyaYolu()); // Mevcut geçmiş listesi diskten okunur
        std::vector<SayfaKaydi> korunacaklar; // Belirtilen zamandan daha eski olduğu için silinmeyecek kayıtlar
        for (const SayfaKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            // ISO tarih biçimi (YYYY-MM-DDTHH:MM:SSZ) sıfır dolgulu olduğu için, metin karşılaştırması aynı zamanda doğru bir kronolojik karşılaştırmadır
            if (kayit.zaman < baslangicZamaniIso) // Eğer kayıt, seçilen zaman aralığından daha eskiyse
            {
                korunacaklar.push_back(kayit); // Bu kayıt silinmeyip korunacaklar listesine eklenir
            }
        }
        DosyayaYaz(GecmisDosyaYolu(), korunacaklar); // Sadece eski kayıtların kaldığı liste, dosyanın üzerine yazılır
    }

    // Bu fonksiyon, GecmisiTemizle ile aynı şekilde çalışır, ancak hariç tutulan sitelerden gelen kayıtları zaman aralığına bakmaksızın korur
    void GecmisiTemizleHaricTutarak(const std::wstring& baslangicZamaniIso, const std::vector<std::wstring>& haricTutulanSiteler)
    {
        if (haricTutulanSiteler.empty()) // Hariç tutulan hiçbir site yoksa, davranış normal temizlemeyle birebir aynıdır
        {
            GecmisiTemizle(baslangicZamaniIso);
            return;
        }

        std::vector<SayfaKaydi> liste = DosyadanOku(GecmisDosyaYolu()); // Mevcut geçmiş listesi diskten okunur
        std::vector<SayfaKaydi> korunacaklar; // Silinmeyip kalacak kayıtlar
        for (const SayfaKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            bool haricTutulduMu = false; // Bu kaydın hariç tutulan sitelerden birine ait olup olmadığı
            for (const std::wstring& site : haricTutulanSiteler)
            {
                if (kayit.adres.rfind(site, 0) == 0) { haricTutulduMu = true; break; } // Adres, hariç tutulan site önekiyle başlıyorsa bu siteye ait kabul edilir
            }

            bool zamanDisindaMi = baslangicZamaniIso.empty() ? false : (kayit.zaman < baslangicZamaniIso); // Kayıt, seçilen zaman aralığının dışında (yani daha eski) mı?
            if (haricTutulduMu || zamanDisindaMi) // Hariç tutulan bir siteye aitse veya zaten zaman aralığının dışındaysa korunur
            {
                korunacaklar.push_back(kayit);
            }
        }
        DosyayaYaz(GecmisDosyaYolu(), korunacaklar); // Korunacak kayıtların kaldığı liste, dosyanın üzerine yazılır
    }

    // Bu fonksiyon, geçmişten adres ve zamanı birebir eşleşen tek bir kaydı siler
    void GecmistenKaydiSil(const std::wstring& adres, const std::wstring& zaman)
    {
        std::vector<SayfaKaydi> liste = DosyadanOku(GecmisDosyaYolu()); // Mevcut geçmiş listesi diskten okunur

        std::vector<SayfaKaydi> yeniListe; // Silinecek kayıt hariç tutularak yeniden oluşturulacak liste
        for (const SayfaKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            if (!(kayit.adres == adres && kayit.zaman == zaman)) // Eğer bu kayıt, hem adresi hem zamanı silinmek istenenle eşleşmiyorsa
            {
                yeniListe.push_back(kayit); // Kayıt, yeni listeye aktarılır
            }
        }

        DosyayaYaz(GecmisDosyaYolu(), yeniListe); // Silinen kayıt hariç yeni liste, diske yazılır
    }

    // Bu fonksiyon, oturum dosyasının tam yolunu döndürür
    static std::wstring OturumDosyaYolu()
    {
        return VeriKlasoruAl() + L"\\session.json"; // Veri klasörünün altındaki session.json dosyasının yolu döndürülür
    }

    // Bu fonksiyon, şu anda açık olan sekmelerin listesini diske kaydeder
    void OturumuKaydet(const std::vector<SayfaKaydi>& acikSekmeler)
    {
        DosyayaYaz(OturumDosyaYolu(), acikSekmeler); // Liste, olduğu gibi (zaman/klasör alanları kullanılmadan) diske yazılır
    }

    // Bu fonksiyon, en son kaydedilmiş oturumdaki sekmeleri bir liste olarak döndürür
    std::vector<SayfaKaydi> OturumuOku()
    {
        return DosyadanOku(OturumDosyaYolu()); // Oturum dosyası okunup, içindeki liste olduğu gibi döndürülür
    }
}
