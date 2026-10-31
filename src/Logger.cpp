// Bu dosya, Logger.h içinde tanımı yapılan hata kaydetme ve hata kodu açıklama fonksiyonlarının gerçek çalışma mantığını içerir.
// Burada, hataların diske nasıl satır satır (JSON Lines biçiminde) yazıldığı, ve hata kodlarının gömülü sözlükte nasıl açıklamalara çevrildiği yazılıdır.
// Bu dosya, Storage.cpp dosyasındaki veri klasörü bulma mantığına benzer bir mantıkla, günlük dosyalarını kullanıcının AppData klasörü altında tutar.

#include "Logger.h" // Bu dosyanın, hangi fonksiyonların gövdesini yazdığını bilmesi için kendi başlık dosyası eklenir
#include "JsonYardimci.h" // Metinleri JSON içine yazma ve JSON nesnesinden alan okuma yardımcıları için eklenir
#include "VeriYolu.h" // Verilerin saklanacağı ana klasörün yolunu bulan ortak fonksiyon için eklenir
#include "Utf8Yardimci.h" // Dosyaları UTF-8 kodlamasıyla okuyup yazan ortak yardımcı fonksiyonlar için eklenir, Türkçe karakterlerin doğru saklanmasını sağlar
#include <windows.h> // Klasör oluşturma, dosya listeleme ve zaman okuma gibi Windows fonksiyonlarını kullanmak için eklenir
#include <sstream> // Satır metinlerini bir araya getirmek için eklenir
#include <map> // Hata kodu sözlüğünü tutmak için eklenir
#include <algorithm> // Dosya adlarını tarihe göre sıralamak için eklenir

namespace Gunluk // Storage.cpp'deki yapıya benzer şekilde, tüm fonksiyonlar bu isim alanının içine konur
{
    // Bu fonksiyon, günlük dosyalarının saklanacağı klasörün tam yolunu döndürür, klasör yoksa önce oluşturur
    static std::wstring GunlukKlasorunuAl()
    {
        std::wstring klasorYolu = VeriKlasoruAl() + L"\\logs"; // Ana veri klasörünün altına, günlük dosyaları için ayrılmış bir alt klasör yolu oluşturulur
        CreateDirectoryW(klasorYolu.c_str(), nullptr); // Bu klasör zaten varsa hiçbir şey yapmaz, yoksa oluşturur
        return klasorYolu; // Hazırlanan günlük klasörü yolu döndürülür
    }

    // Bu fonksiyon, şu anki tarihi, dosya adında kullanılabilecek YYYY-MM-DD biçiminde bir metin olarak döndürür
    static std::wstring BugununTarihiMetinOlarak()
    {
        SYSTEMTIME zaman; // Şu anki zamanı tutacak Windows yapısı
        GetLocalTime(&zaman); // Sistemin şu anki yerel saati bu yapıya okunur
        wchar_t tamponMetin[16]; // Tarihin metin haline yazılacağı tampon
        swprintf_s(tamponMetin, L"%04d-%02d-%02d", zaman.wYear, zaman.wMonth, zaman.wDay); // Yıl, ay ve gün bilgileri birleştirilir
        return tamponMetin; // Hazırlanan tarih metni döndürülür
    }

    // Bu fonksiyon, şu anki saati, günlük satırının içinde kullanılacak saat:dakika:saniye biçiminde bir metin olarak döndürür
    static std::wstring SuankiSaatMetinOlarak()
    {
        SYSTEMTIME zaman; // Şu anki zamanı tutacak Windows yapısı
        GetLocalTime(&zaman); // Sistemin şu anki yerel saati bu yapıya okunur
        wchar_t tamponMetin[16]; // Saatin metin haline yazılacağı tampon
        swprintf_s(tamponMetin, L"%02d:%02d:%02d", zaman.wHour, zaman.wMinute, zaman.wSecond); // Saat, dakika ve saniye bilgileri birleştirilir
        return tamponMetin; // Hazırlanan saat metni döndürülür
    }

    // Bu fonksiyon, verilen hata kodunu ve mesajını, bugünün tarihine ait günlük dosyasının sonuna bir satır olarak ekler
    void HataKaydet(const std::wstring& kod, const std::wstring& mesaj)
    {
        std::wstring dosyaYolu = GunlukKlasorunuAl() + L"\\app-" + BugununTarihiMetinOlarak() + L".jsonl"; // Bugüne ait günlük dosyasının tam yolu hazırlanır

        // Hata, saat, kod ve mesaj alanlarını içeren tek satırlık bir JSON nesnesi hazırlanır, satır sonunda yeni satır karakteri bırakılır
        std::wstring satir = L"{\"saat\":\"" + SuankiSaatMetinOlarak() + L"\",\"kod\":\"" + JsonIcinKacisla(kod) + L"\",\"mesaj\":\"" + JsonIcinKacisla(mesaj) + L"\"}\n";
        Utf8DosyaYaz(dosyaYolu, satir, true); // Hazırlanan satır, dosyanın sonuna UTF-8 kodlamasıyla eklenir
    }

    // Bu fonksiyon, bilinen hata kodlarını kısa açıklamalarına eşleyen gömülü sözlüğü döndürür, sözlük her çağrıda yeniden oluşturulmasın diye sadece bir kere hazırlanır
    static const std::map<std::wstring, std::wstring>& HataKoduSozlugu()
    {
        static const std::map<std::wstring, std::wstring> sozluk = {
            { L"NET-1001", L"Sayfanın adresi bulunamadı, adresi kontrol edin." }, // İnternet adresi (alan adı) çözümlenemediğinde kullanılan kod
            { L"NET-1002", L"Bağlantı reddedildi, sunucuya ulaşılamadı." }, // Hedef sunucu bağlantıyı kabul etmediğinde kullanılan kod
            { L"NET-1003", L"Bağlantı zaman aşımına uğradı." }, // Sayfa yüklemesi çok uzun sürüp zaman aşımına uğradığında kullanılan kod
            { L"NET-1004", L"İnternet bağlantısı yok gibi görünüyor." }, // Bilgisayarın ağ bağlantısı tamamen kesikken kullanılan kod
            { L"NET-1005", L"Bu sitenin güvenlik sertifikasında bir sorun var." }, // Sitenin SSL sertifikası geçersiz veya süresi dolmuşken kullanılan kod
            { L"NET-1099", L"Sayfa bilinmeyen bir sebeple yüklenemedi." }, // Yukarıdaki kategorilerin hiçbirine uymayan ağ hataları için kullanılan genel kod
            { L"UI-2010", L"Bu sekmenin içeriği beklenmedik şekilde kapandı." }, // Bir sekmenin render sürecinin çöktüğü durumlarda kullanılan kod
            { L"FS-4001", L"Kayıtlı veriler okunurken bir sorun oluştu." }, // history.json veya bookmarks.json gibi dosyalar okunamadığında kullanılan kod
        };
        return sozluk; // Hazırlanan sabit sözlük döndürülür
    }

    // Bu fonksiyon, verilen bir hata koduna karşılık gelen açıklamayı sözlükten bulup döndürür
    std::wstring KoduAciklamayaCevir(const std::wstring& kod)
    {
        const std::map<std::wstring, std::wstring>& sozluk = HataKoduSozlugu(); // Gömülü hata kodu sözlüğüne erişilir
        auto bulunan = sozluk.find(kod); // Verilen kod, sözlükte aranır
        if (bulunan != sozluk.end()) // Eğer kod sözlükte bulunduysa
        {
            return bulunan->second; // Bu koda karşılık gelen açıklama döndürülür
        }
        return L"Beklenmedik bir hata oluştu."; // Kod sözlükte bulunamadıysa, genel bir açıklama döndürülür
    }

    // Bu fonksiyon, günlük klasöründeki tüm "app-TARİH.jsonl" dosyalarının adlarını, en yeni tarihten en eski tarihe doğru sıralanmış şekilde bulup döndürür
    static std::vector<std::wstring> GunlukDosyaAdlariniBul()
    {
        std::vector<std::wstring> dosyaAdlari; // Bulunan dosya adlarını tutacak liste

        std::wstring aramaDeseni = GunlukKlasorunuAl() + L"\\app-*.jsonl"; // Günlük klasöründeki, bu isim kalıbına uyan tüm dosyalar aranacak
        WIN32_FIND_DATAW bulunanDosya; // Bulunan her dosyanın bilgilerini tutacak yapı
        HANDLE aramaTanitici = FindFirstFileW(aramaDeseni.c_str(), &bulunanDosya); // Bu kalıba uyan ilk dosya aranır

        if (aramaTanitici != INVALID_HANDLE_VALUE) // Eğer en az bir dosya bulunduysa
        {
            do // Bulunan her dosya için
            {
                dosyaAdlari.push_back(bulunanDosya.cFileName); // Dosyanın adı listeye eklenir
            } while (FindNextFileW(aramaTanitici, &bulunanDosya)); // Bir sonraki dosya aranır, bulunamayana kadar devam edilir
            FindClose(aramaTanitici); // Arama tanıtıcısı artık kullanılmadığı için kapatılır
        }

        std::sort(dosyaAdlari.begin(), dosyaAdlari.end(), std::greater<std::wstring>()); // Dosya adları, tarih bilgisi isimde olduğu için alfabetik tersten sıralanınca en yeni tarih en başa gelir

        return dosyaAdlari; // Bulunup sıralanmış dosya adları döndürülür
    }

    // Bu fonksiyon, en güncel günlük dosyalarından, en yeniden en eskiye doğru sıralanmış en fazla "azami" sayıda hata kaydını okuyup döndürür
    std::vector<HataGirdisi> SonHatalariOku(int azami)
    {
        std::vector<HataGirdisi> sonuc; // Döndürülecek sonuç listesi

        std::vector<std::wstring> dosyaAdlari = GunlukDosyaAdlariniBul(); // Günlük klasöründeki dosya adları, en yeniden en eskiye doğru bulunur

        for (const std::wstring& dosyaAdi : dosyaAdlari) // Her günlük dosyası, en yeniden başlanarak sırayla gezilir
        {
            if (static_cast<int>(sonuc.size()) >= azami) // Eğer istenen sayıda kayıt zaten toplandıysa
            {
                break; // Daha fazla dosya okumaya gerek yoktur
            }

            // Dosya adından ("app-2026-10-29.jsonl" gibi) sadece tarih kısmı ("2026-10-29") ayıklanır
            std::wstring tarih = dosyaAdi.substr(4, dosyaAdi.size() - 4 - 6); // "app-" önekinin dört karakteri ve ".jsonl" sonekinin altı karakteri atlanır

            std::wstring dosyaIcerigi = Utf8DosyaOku(GunlukKlasorunuAl() + L"\\" + dosyaAdi); // Bu günlük dosyasının tüm içeriği, UTF-8 kodlaması çözülerek okunur
            if (dosyaIcerigi.empty()) // Eğer dosya açılamadıysa veya tamamen boşsa
            {
                continue; // Bu dosya atlanıp bir sonraki dosyaya geçilir
            }

            std::vector<std::wstring> satirlar; // Bu dosyadaki tüm satırları tutacak liste
            std::wstringstream akis(dosyaIcerigi); // Okunan içerik, satır satır gezilebilmesi için bir metin akışına sarılır
            std::wstring satir; // Akıştan okunan her satırı tutacak geçici değişken
            while (std::getline(akis, satir)) // İçeriğin her satırı sırayla okunur
            {
                if (!satir.empty()) // Boş satırlar (örneğin dosya sonundaki fazladan yeni satır) atlanır
                {
                    satirlar.push_back(satir); // Satır, listeye eklenir
                }
            }

            // Satırlar, dosyada en son yazılan (yani en yeni) satırdan başlanarak geriye doğru gezilir, böylece en yeni hatalar listenin başında olur
            for (auto satirIterator = satirlar.rbegin(); satirIterator != satirlar.rend(); ++satirIterator)
            {
                if (static_cast<int>(sonuc.size()) >= azami) // Eğer istenen sayıya ulaşıldıysa
                {
                    break; // Bu dosyadan daha fazla satır okumaya gerek yoktur
                }

                HataGirdisi girdi; // Bu satırdan oluşturulacak yeni bir günlük girdisi
                girdi.tarih = tarih; // Dosya adından ayıklanan tarih atanır
                girdi.saat = JsonAlaniBul(*satirIterator, L"saat"); // Satırın saat alanı okunur
                girdi.kod = JsonAlaniBul(*satirIterator, L"kod"); // Satırın kod alanı okunur
                girdi.mesaj = JsonAlaniBul(*satirIterator, L"mesaj"); // Satırın mesaj alanı okunur

                if (!girdi.kod.empty()) // Eğer bu satırdan geçerli bir kod okunabildiyse, yani satır gerçekten bir hata kaydı gibi görünüyorsa
                {
                    sonuc.push_back(girdi); // Girdi, sonuç listesine eklenir
                }
            }
        }

        return sonuc; // Toplanan hata kayıtları döndürülür
    }
}
