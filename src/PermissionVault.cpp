// Bu dosya, PermissionVault.h içinde tanımı yapılan izin kararı kaydetme ve okuma fonksiyonlarının gerçek çalışma mantığını içerir.
// Kararlar, Storage.cpp'deki geçmiş ve yer imi dosyalarıyla aynı basit, kendi yazdığımız JSON benzeri biçimde, permissions.json dosyasında tutulur.

#include "PermissionVault.h" // Bu dosyanın, hangi fonksiyonların gövdesini yazdığını bilmesi için kendi başlık dosyası eklenir
#include "JsonYardimci.h" // Metinleri JSON içine yazma, JSON'dan çözme ve JSON nesnesinden alan okuma yardımcıları için eklenir
#include "VeriYolu.h" // Verilerin saklanacağı ana klasörün yolunu bulan ortak fonksiyon için eklenir
#include "Utf8Yardimci.h" // Dosyaları UTF-8 kodlamasıyla okuyup yazan ortak yardımcı fonksiyonlar için eklenir

namespace IzinKasasi // BrowserWindow.cpp dosyasındaki isim alanıyla aynı isim alanı burada da kullanılır, çünkü aynı fonksiyonların gövdeleri yazılıyor
{
    // Bu fonksiyon, izin kararları dosyasının tam yolunu döndürür
    static std::wstring IzinDosyaYolu()
    {
        return VeriKlasoruAl() + L"\\permissions.json"; // Ana veri klasörünün altındaki permissions.json dosyasının yolu döndürülür
    }

    // Bu fonksiyon, bir izin kaydı listesini, dosyaya yazılabilecek bir JSON dizisi metnine çevirir
    static std::wstring ListeyiJsonYap(const std::vector<IzinKaydi>& liste)
    {
        std::wstring sonuc = L"["; // JSON dizisinin açılış köşeli parantezi ile başlanır
        for (size_t i = 0; i < liste.size(); i++) // Listedeki her kayıt sırayla gezilir
        {
            if (i > 0) { sonuc += L","; } // İlk kayıttan sonraki her kayıttan önce virgül eklenir
            sonuc += L"{\"kaynak\":\"" + JsonIcinKacisla(liste[i].kaynak) + L"\",\"tur\":\"" + JsonIcinKacisla(liste[i].tur) + L"\",\"izinVerildi\":\"" + (liste[i].izinVerildi ? L"evet" : L"hayir") + L"\"}"; // Her kayıt bir JSON nesnesi olarak yazılır
        }
        sonuc += L"]"; // JSON dizisinin kapanış köşeli parantezi eklenir
        return sonuc; // Hazırlanan JSON dizisi metni döndürülür
    }

    // Bu fonksiyon, dosyadan okunan bir JSON dizisi metnini, bir izin kaydı listesine çevirir
    static std::vector<IzinKaydi> JsonDiziyiListeYap(const std::wstring& jsonMetni)
    {
        std::vector<IzinKaydi> liste; // Sonuç olarak döndürülecek boş bir liste oluşturulur

        size_t konum = 0; // Metin içinde ilerlerken kullanılacak konum sayacı
        while (true) // Metindeki tüm nesneler bulunana kadar devam eden bir döngü
        {
            size_t nesneBaslangici = jsonMetni.find(L'{', konum); // Bir sonraki JSON nesnesinin açılış süslü parantezi aranır
            if (nesneBaslangici == std::wstring::npos) { break; } // Daha fazla nesne bulunamadıysa döngüden çıkılır
            size_t nesneBitisi = jsonMetni.find(L'}', nesneBaslangici); // Bu nesnenin kapanış süslü parantezi aranır
            if (nesneBitisi == std::wstring::npos) { break; } // Kapanış bulunamadıysa, dosya bozuk demektir, döngüden çıkılır

            std::wstring nesneMetni = jsonMetni.substr(nesneBaslangici, nesneBitisi - nesneBaslangici + 1); // Bu tek nesnenin tam metni ayıklanır

            IzinKaydi kayit; // Bu nesneden oluşturulacak yeni bir kayıt
            kayit.kaynak = JsonAlaniBul(nesneMetni, L"kaynak"); // Nesnenin kaynak alanı okunur
            kayit.tur = JsonAlaniBul(nesneMetni, L"tur"); // Nesnenin tür alanı okunur
            kayit.izinVerildi = (JsonAlaniBul(nesneMetni, L"izinVerildi") == L"evet"); // Nesnenin karar alanı okunur

            if (!kayit.kaynak.empty() && !kayit.tur.empty()) // Eğer bu nesneden geçerli bir kayıt okunabildiyse
            {
                liste.push_back(kayit); // Kayıt, sonuç listesine eklenir
            }

            konum = nesneBitisi + 1; // Arama konumu, bu nesnenin bittiği yerin bir ötesine taşınır
        }

        return liste; // Dosyadan okunup listeye çevrilmiş kayıtlar döndürülür
    }

    // Bu fonksiyon, izin kararları dosyasını okuyup bir kayıt listesine çevirir, dosya yoksa veya bozuksa boş bir liste döndürür
    static std::vector<IzinKaydi> DosyadanOku()
    {
        std::wstring icerik = Utf8DosyaOku(IzinDosyaYolu()); // Dosyanın tüm içeriği, UTF-8 kodlaması çözülerek okunur
        return JsonDiziyiListeYap(icerik); // Okunan metin, bir kayıt listesine çevrilip döndürülür
    }

    // Bu fonksiyon, verilen bir izin kaydı listesini, izin kararları dosyasına JSON biçiminde yazar
    static void DosyayaYaz(const std::vector<IzinKaydi>& liste)
    {
        Utf8DosyaYaz(IzinDosyaYolu(), ListeyiJsonYap(liste)); // Liste, JSON metnine çevrilip, UTF-8 kodlamasıyla dosyaya yazılır
    }

    bool KararBul(const std::wstring& kaynak, const std::wstring& tur, bool& izinVerildi)
    {
        std::vector<IzinKaydi> liste = DosyadanOku(); // Mevcut izin listesi diskten okunur
        for (const IzinKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            if (kayit.kaynak == kaynak && kayit.tur == tur) // Eğer bu kayıt, aranan kaynak ve türle eşleşiyorsa
            {
                izinVerildi = kayit.izinVerildi; // Kaydedilmiş karar, çıktı parametresine yazılır
                return true; // Bir karar bulunduğu bildirilir
            }
        }
        return false; // Bu kaynak ve tür için hiç kayıtlı bir karar bulunamadı
    }

    void KararKaydet(const std::wstring& kaynak, const std::wstring& tur, bool izinVerildi)
    {
        std::vector<IzinKaydi> liste = DosyadanOku(); // Mevcut izin listesi diskten okunur

        for (IzinKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            if (kayit.kaynak == kaynak && kayit.tur == tur) // Eğer bu kaynak ve tür için zaten bir kayıt varsa
            {
                kayit.izinVerildi = izinVerildi; // Mevcut kaydın kararı güncellenir
                DosyayaYaz(liste); // Güncellenmiş liste diske yazılır
                return; // İşlem tamamlandı, yeni bir kayıt eklemeye gerek yok
            }
        }

        IzinKaydi yeniKayit; // Eklenecek yeni kayıt hazırlanır
        yeniKayit.kaynak = kaynak;
        yeniKayit.tur = tur;
        yeniKayit.izinVerildi = izinVerildi;
        liste.push_back(yeniKayit); // Yeni kayıt, listenin sonuna eklenir
        DosyayaYaz(liste); // Güncellenmiş liste diske yazılır
    }

    void KararSil(const std::wstring& kaynak, const std::wstring& tur)
    {
        std::vector<IzinKaydi> liste = DosyadanOku(); // Mevcut izin listesi diskten okunur

        std::vector<IzinKaydi> yeniListe; // Silinecek kayıt hariç tutularak yeniden oluşturulacak liste
        for (const IzinKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            if (!(kayit.kaynak == kaynak && kayit.tur == tur)) // Eğer bu kayıt, silinmek istenen kaynak ve türle eşleşmiyorsa
            {
                yeniListe.push_back(kayit); // Kayıt, yeni listeye aktarılır
            }
        }

        DosyayaYaz(yeniListe); // Silinen kayıt hariç yeni liste, diske yazılır
    }

    std::vector<IzinKaydi> TumKararlariOku()
    {
        return DosyadanOku(); // İzin dosyası okunup, içindeki liste olduğu gibi döndürülür
    }
}
