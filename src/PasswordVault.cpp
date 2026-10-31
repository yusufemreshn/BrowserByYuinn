// Bu dosya, PasswordVault.h içinde tanımı yapılan şifre yöneticisi fonksiyonlarının gerçek çalışma mantığını içerir.
// Şifreler, Windows'un DPAPI (Data Protection API) adlı sistemiyle şifrelenir, bu sistem Windows'un kendi kullanıcı oturum anahtarını kullanır, bu yüzden başka bir programın veya başka bir bilgisayarın bu şifreyi çözmesi mümkün değildir.
// Şifrelenmiş ham baytlar, bir JSON dosyasının içine yazılabilmesi için Base64Yardimci.h'deki fonksiyonlarla metne çevrilir.

#include "PasswordVault.h" // Bu dosyanın, hangi fonksiyonların gövdesini yazdığını bilmesi için kendi başlık dosyası eklenir
#include "JsonYardimci.h" // Metinleri JSON içine yazma ve JSON nesnesinden alan okuma yardımcıları için eklenir
#include "VeriYolu.h" // Verilerin saklanacağı ana klasörün yolunu bulan ortak fonksiyon için eklenir
#include "Base64Yardimci.h" // Şifrelenmiş ham baytları metne çevirmek ve geri çözmek için eklenir
#include "Utf8Yardimci.h" // Dosyaları UTF-8 kodlamasıyla okuyup yazan ortak yardımcı fonksiyonlar için eklenir, Türkçe karakterlerin doğru saklanmasını sağlar
#include <windows.h> // Zaman bilgisini okumak için Windows fonksiyonlarını kullanmak için eklenir
#include <wincrypt.h> // DPAPI'nin CryptProtectData ve CryptUnprotectData fonksiyonlarını kullanmak için eklenir

namespace SifreKasasi // Tüm şifre yöneticisi fonksiyonları, başka isimlerle çakışmaması için bu isim alanının içine konur
{
    // Bu fonksiyon, şifrelerin saklandığı passwords.json dosyasının tam yolunu döndürür
    static std::wstring SifreDosyaYolu()
    {
        return VeriKlasoruAl() + L"\\passwords.json"; // Ana veri klasörünün altındaki passwords.json dosyasının yolu döndürülür
    }

    // Bu fonksiyon, şu anki tarih ve saati, ISO biçiminde bir metin olarak döndürür
    static std::wstring SuankiZamaniAl()
    {
        SYSTEMTIME zaman; // Şu anki zamanı tutacak Windows yapısı
        GetSystemTime(&zaman); // Sistemin şu anki evrensel saati bu yapıya okunur
        wchar_t tamponMetin[32]; // Zamanın metin haline yazılacağı tampon
        swprintf_s(tamponMetin, L"%04d-%02d-%02dT%02d:%02d:%02dZ", zaman.wYear, zaman.wMonth, zaman.wDay, zaman.wHour, zaman.wMinute, zaman.wSecond); // Yıl, ay, gün, saat, dakika, saniye bilgileri ISO biçiminde birleştirilir
        return tamponMetin; // Hazırlanan zaman metni döndürülür
    }

    // Bu fonksiyon, verilen düz metin şifreyi, Windows'un DPAPI sistemiyle şifreleyip, sonucu Base64 ile kodlanmış bir metin olarak döndürür
    static std::wstring DuzMetniSifrele(const std::wstring& duzMetin)
    {
        DATA_BLOB girdi; // Şifrelenecek ham veriyi DPAPI'nin beklediği biçimde tutacak yapı
        girdi.pbData = reinterpret_cast<BYTE*>(const_cast<wchar_t*>(duzMetin.data())); // Metnin ham bayt baştan işaretçisi verilir
        girdi.cbData = static_cast<DWORD>(duzMetin.size() * sizeof(wchar_t)); // Metnin bayt cinsinden uzunluğu hesaplanır, her karakter iki bayttır

        DATA_BLOB cikti = {}; // Şifrelenmiş sonucu tutacak boş bir yapı
        // CRYPTPROTECT_UI_FORBIDDEN bayrağı, şifreleme sırasında Windows'un kendi bir ekran göstermesini engeller, bu işlem sessizce arka planda olur
        BOOL basarili = CryptProtectData(&girdi, L"TarayiciSifre", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &cikti);

        if (!basarili) // Eğer şifreleme başarısız olduysa
        {
            return L""; // Boş bir metin döndürülür, çağıran taraf bunu hata olarak yorumlayabilir
        }

        std::vector<uint8_t> hamVeri(cikti.pbData, cikti.pbData + cikti.cbData); // Şifrelenmiş ham baytlar, bir listeye kopyalanır
        LocalFree(cikti.pbData); // DPAPI'nin bizim için ayırdığı bellek serbest bırakılır

        return Base64Kodla(hamVeri); // Şifrelenmiş ham baytlar, dosyaya yazılabilecek bir metne çevrilip döndürülür
    }

    // Bu fonksiyon, verilen şifreli ve Base64 kodlu bir veriyi, DPAPI ile çözüp düz metin şifreyi döndürür, çözme başarısız olursa boş bir metin döndürür
    std::wstring SifreyiCoz(const std::wstring& sifreliVeri)
    {
        std::vector<uint8_t> hamVeri = Base64Coz(sifreliVeri); // Base64 ile kodlanmış metin, tekrar ham baytlara çevrilir
        if (hamVeri.empty()) // Eğer çözülecek bir veri yoksa
        {
            return L""; // Boş bir metin döndürülür
        }

        DATA_BLOB girdi; // Çözülecek şifreli veriyi DPAPI'nin beklediği biçimde tutacak yapı
        girdi.pbData = hamVeri.data(); // Şifreli baytların başlangıç adresi verilir
        girdi.cbData = static_cast<DWORD>(hamVeri.size()); // Şifreli baytların uzunluğu verilir

        DATA_BLOB cikti = {}; // Çözülmüş sonucu tutacak boş bir yapı
        BOOL basarili = CryptUnprotectData(&girdi, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &cikti); // Veri, aynı Windows kullanıcı hesabının anahtarıyla çözülmeye çalışılır

        if (!basarili) // Eğer çözme başarısız olduysa, örneğin veri başka bir bilgisayardan kopyalandıysa
        {
            return L""; // Boş bir metin döndürülür
        }

        std::wstring sonuc(reinterpret_cast<wchar_t*>(cikti.pbData), cikti.cbData / sizeof(wchar_t)); // Çözülen ham baytlar, tekrar bir metin olarak yorumlanır
        LocalFree(cikti.pbData); // DPAPI'nin bizim için ayırdığı bellek serbest bırakılır

        return sonuc; // Çözülmüş düz metin şifre döndürülür
    }

    // Bu fonksiyon, bir kayıt listesini, dosyaya yazılabilecek bir JSON dizisi metnine çevirir
    static std::wstring ListeyiJsonYap(const std::vector<SifreKaydi>& liste)
    {
        std::wstring sonuc = L"["; // JSON dizisinin açılış köşeli parantezi ile başlanır
        for (size_t i = 0; i < liste.size(); i++) // Listedeki her kayıt sırayla gezilir
        {
            if (i > 0) { sonuc += L","; } // İlk kayıttan sonraki her kayıttan önce virgül eklenir
            sonuc += L"{\"site\":\"" + JsonIcinKacisla(liste[i].site) + L"\",\"kullaniciAdi\":\"" + JsonIcinKacisla(liste[i].kullaniciAdi) + L"\",\"sifreliVeri\":\"" + JsonIcinKacisla(liste[i].sifreliVeri) + L"\",\"eklenmeZamani\":\"" + JsonIcinKacisla(liste[i].eklenmeZamani) + L"\"}";
        }
        sonuc += L"]"; // JSON dizisinin kapanış köşeli parantezi eklenir
        return sonuc; // Hazırlanan JSON dizisi metni döndürülür
    }

    // Bu fonksiyon, dosyadan okunan bir JSON dizisi metnini, bir şifre kaydı listesine çevirir
    static std::vector<SifreKaydi> JsonDiziyiListeYap(const std::wstring& jsonMetni)
    {
        std::vector<SifreKaydi> liste; // Sonuç olarak döndürülecek boş bir liste oluşturulur

        size_t konum = 0; // Metin içinde ilerlerken kullanılacak konum sayacı
        while (true) // Metindeki tüm nesneler bulunana kadar devam eden bir döngü
        {
            size_t nesneBaslangici = jsonMetni.find(L'{', konum); // Bir sonraki JSON nesnesinin açılış süslü parantezi aranır
            if (nesneBaslangici == std::wstring::npos) { break; } // Daha fazla nesne bulunamadıysa döngüden çıkılır
            size_t nesneBitisi = jsonMetni.find(L'}', nesneBaslangici); // Bu nesnenin kapanış süslü parantezi aranır
            if (nesneBitisi == std::wstring::npos) { break; } // Kapanış bulunamadıysa, dosya bozuk demektir, döngüden çıkılır

            std::wstring nesneMetni = jsonMetni.substr(nesneBaslangici, nesneBitisi - nesneBaslangici + 1); // Bu tek nesnenin tam metni ayıklanır

            SifreKaydi kayit; // Bu nesneden oluşturulacak yeni bir kayıt
            kayit.site = JsonAlaniBul(nesneMetni, L"site"); // Nesnenin site alanı okunur
            kayit.kullaniciAdi = JsonAlaniBul(nesneMetni, L"kullaniciAdi"); // Nesnenin kullanıcı adı alanı okunur
            kayit.sifreliVeri = JsonAlaniBul(nesneMetni, L"sifreliVeri"); // Nesnenin şifreli veri alanı okunur
            kayit.eklenmeZamani = JsonAlaniBul(nesneMetni, L"eklenmeZamani"); // Nesnenin eklenme zamanı alanı okunur

            if (!kayit.site.empty()) // Eğer bu nesneden geçerli bir site okunabildiyse
            {
                liste.push_back(kayit); // Kayıt, sonuç listesine eklenir
            }

            konum = nesneBitisi + 1; // Arama konumu, bu nesnenin bittiği yerin bir ötesine taşınır
        }

        return liste; // Dosyadan okunup listeye çevrilmiş kayıtlar döndürülür
    }

    // Bu fonksiyon, verilen dosya yolundaki JSON dosyasını okuyup bir şifre kaydı listesine çevirir, dosya yoksa veya bozuksa boş bir liste döndürür
    static std::vector<SifreKaydi> DosyadanOku()
    {
        std::wstring icerik = Utf8DosyaOku(SifreDosyaYolu()); // Dosyanın tüm içeriği, UTF-8 kodlaması çözülerek bir metin olarak okunur
        return JsonDiziyiListeYap(icerik); // Okunan metin, bir kayıt listesine çevrilip döndürülür
    }

    // Bu fonksiyon, verilen bir şifre kaydı listesini, şifre dosyasına JSON biçiminde yazar
    static void DosyayaYaz(const std::vector<SifreKaydi>& liste)
    {
        Utf8DosyaYaz(SifreDosyaYolu(), ListeyiJsonYap(liste)); // Liste, JSON metnine çevrilip, UTF-8 kodlamasıyla dosyaya yazılır
    }

    // Bu fonksiyon, verilen site, kullanıcı adı ve düz metin şifreyi alıp, şifreyi DPAPI ile şifreleyerek yeni bir kayıt olarak diske ekler
    void SifreEkle(const std::wstring& site, const std::wstring& kullaniciAdi, const std::wstring& duzSifre)
    {
        std::vector<SifreKaydi> liste = DosyadanOku(); // Mevcut şifre listesi diskten okunur

        SifreKaydi yeniKayit; // Eklenecek yeni kayıt hazırlanır
        yeniKayit.site = site; // Sitenin adresi atanır
        yeniKayit.kullaniciAdi = kullaniciAdi; // Kullanıcı adı atanır
        yeniKayit.sifreliVeri = DuzMetniSifrele(duzSifre); // Düz metin şifre, DPAPI ile şifrelenip kaydedilir, düz metin hiçbir zaman diske yazılmaz
        yeniKayit.eklenmeZamani = SuankiZamaniAl(); // Şu anki zaman, eklenme zamanı olarak atanır

        liste.push_back(yeniKayit); // Yeni kayıt, listenin sonuna eklenir
        DosyayaYaz(liste); // Güncellenmiş liste, tekrar diske yazılır
    }

    // Bu fonksiyon, verilen site ve kullanıcı adına sahip kaydı siler
    void SifreSil(const std::wstring& site, const std::wstring& kullaniciAdi)
    {
        std::vector<SifreKaydi> liste = DosyadanOku(); // Mevcut şifre listesi diskten okunur

        std::vector<SifreKaydi> yeniListe; // Silinecek kayıt hariç tutularak yeniden oluşturulacak liste
        for (const SifreKaydi& kayit : liste) // Listedeki her kayıt kontrol edilir
        {
            if (!(kayit.site == site && kayit.kullaniciAdi == kullaniciAdi)) // Eğer bu kayıt, silinmek istenen site ve kullanıcı adıyla eşleşmiyorsa
            {
                yeniListe.push_back(kayit); // Kayıt, yeni listeye aktarılır
            }
        }

        DosyayaYaz(yeniListe); // Silinen kayıt hariç yeni liste, diske yazılır
    }

    // Bu fonksiyon, kaydedilmiş tüm şifreleri, şifreli haliyle bir liste olarak döndürür
    std::vector<SifreKaydi> SifreleriOku()
    {
        return DosyadanOku(); // Şifre dosyası okunup, içindeki liste olduğu gibi döndürülür
    }

    // Bu fonksiyon, kayıtlı tüm şifreleri siler, veri temizleme panelinde "Şifreler" kategorisi seçildiğinde kullanılır
    void TumSifreleriSil()
    {
        DosyayaYaz({}); // Şifre dosyasının üzerine boş bir liste yazılır, böylece dosya boşalmış olur
    }

    // Bu fonksiyon, verilen düz metin şifrenin zayıf olup olmadığını tamamen yerel olarak kontrol eder
    bool SifreZayifMi(const std::wstring& duzSifre)
    {
        if (duzSifre.size() < 8) { return true; } // Sekiz karakterden kısa şifreler her zaman zayıf kabul edilir

        bool harfVar = false, rakamVar = false, buyukHarfVar = false; // Şifrenin karakter çeşitliliği burada toplanır
        for (wchar_t karakter : duzSifre)
        {
            if (iswalpha(karakter)) { harfVar = true; if (iswupper(karakter)) { buyukHarfVar = true; } }
            else if (iswdigit(karakter)) { rakamVar = true; }
        }
        if (!harfVar || !rakamVar) { return true; } // Sadece harf veya sadece rakamdan oluşan şifreler zayıf kabul edilir
        if (!buyukHarfVar) { return true; } // Hiç büyük harf içermeyen şifreler de zayıf kabul edilir

        // En sık kullanılan, herkesçe bilinen şifrelerden oluşan küçük, sabit bir liste; şifre (büyük/küçük harf duyarsız) bu listede varsa, yukarıdaki kontrolleri geçse bile zayıf kabul edilir
        static const wchar_t* yayginSifreler[] = {
            L"password", L"password1", L"12345678", L"123456789", L"1234567890", L"qwerty123", L"qwertyuiop",
            L"letmein1", L"welcome1", L"admin123", L"iloveyou", L"princess", L"sunshine", L"football", L"monkey123",
            L"dragon123", L"baseball", L"superman", L"trustno1", L"123123123", L"abc123456", L"password123"
        };
        std::wstring kucukSifre;
        for (wchar_t karakter : duzSifre) { kucukSifre += static_cast<wchar_t>(towlower(karakter)); }
        for (const wchar_t* yaygin : yayginSifreler)
        {
            if (kucukSifre == yaygin) { return true; }
        }

        return false; // Yukarıdaki hiçbir zayıflık kriterine uymuyorsa, şifre yeterince güçlü kabul edilir
    }

    // Bu fonksiyon, verilen düz metin şifrenin SHA-1 özetini, büyük harf onaltılık bir metin olarak döndürür
    std::wstring SifreSha1OzetiAl(const std::wstring& duzSifre)
    {
        std::wstring sonuc; // Hesaplanan özetin onaltılık metni, hata durumunda boş kalır

        // UTF-8, HIBP'nin beklediği bayt kodlamasıdır (şifre sadece ASCII ise fark etmez, ama Türkçe karakter içeren şifrelerde doğru sonuç için önemlidir)
        int gerekliBoyut = WideCharToMultiByte(CP_UTF8, 0, duzSifre.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (gerekliBoyut <= 0) { return sonuc; }
        std::vector<char> utf8Bayt(gerekliBoyut);
        WideCharToMultiByte(CP_UTF8, 0, duzSifre.c_str(), -1, utf8Bayt.data(), gerekliBoyut, nullptr, nullptr);
        int utf8Uzunluk = gerekliBoyut - 1; // WideCharToMultiByte, -1 cchWideChar ile çağrıldığında sondaki null sonlandırıcıyı da sayar, hash hesabına o dahil edilmemeli

        HCRYPTPROV kriptoSaglayici = 0;
        if (!CryptAcquireContextW(&kriptoSaglayici, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) { return sonuc; }

        HCRYPTHASH hashNesnesi = 0;
        if (CryptCreateHash(kriptoSaglayici, CALG_SHA1, 0, 0, &hashNesnesi))
        {
            if (CryptHashData(hashNesnesi, reinterpret_cast<const BYTE*>(utf8Bayt.data()), static_cast<DWORD>(utf8Uzunluk), 0))
            {
                BYTE ozetBaytlari[20]; // SHA-1 özeti her zaman tam olarak yirmi bayttır
                DWORD ozetBoyutu = sizeof(ozetBaytlari);
                if (CryptGetHashParam(hashNesnesi, HP_HASHVAL, ozetBaytlari, &ozetBoyutu, 0))
                {
                    wchar_t onaltilikKarakter[3];
                    for (DWORD i = 0; i < ozetBoyutu; i++)
                    {
                        swprintf_s(onaltilikKarakter, L"%02X", ozetBaytlari[i]); // Her bayt, iki haneli büyük harf onaltılık olarak yazılır
                        sonuc += onaltilikKarakter;
                    }
                }
            }
            CryptDestroyHash(hashNesnesi);
        }
        CryptReleaseContext(kriptoSaglayici, 0);

        return sonuc;
    }
}
