// Bu dosya, AutofillVault.h içinde tanımı yapılan otomatik doldurma kasası fonksiyonlarının gerçek çalışma mantığını içerir.
// Kişisel bilgi profili autofill-profile.json dosyasında, kartlar autofill-cards.json dosyasında, ikisi de PasswordVault.cpp'deki şifreleme/JSON yöntemiyle tutarlı bir biçimde saklanır.

#include "AutofillVault.h" // Bu dosyanın, hangi fonksiyonların gövdesini yazdığını bilmesi için kendi başlık dosyası eklenir
#include "JsonYardimci.h" // Metinleri JSON içine yazma ve JSON nesnesinden alan okuma yardımcıları için eklenir
#include "VeriYolu.h" // Verilerin saklanacağı ana klasörün yolunu bulan ortak fonksiyon için eklenir
#include "Base64Yardimci.h" // Şifrelenmiş ham baytları metne çevirmek ve geri çözmek için eklenir
#include "Utf8Yardimci.h" // Dosyaları UTF-8 kodlamasıyla okuyup yazan ortak yardımcı fonksiyonlar için eklenir
#include <windows.h> // Zaman bilgisini okumak için Windows fonksiyonlarını kullanmak için eklenir
#include <wincrypt.h> // DPAPI'nin CryptProtectData ve CryptUnprotectData fonksiyonlarını kullanmak için eklenir

namespace OtomatikDoldurmaKasasi // Tüm otomatik doldurma kasası fonksiyonları, başka isimlerle çakışmaması için bu isim alanının içine konur
{
    // Bu fonksiyon, kişisel bilgi profilinin saklandığı dosyanın tam yolunu döndürür
    static std::wstring ProfilDosyaYolu()
    {
        return VeriKlasoruAl() + L"\\autofill-profile.json"; // Ana veri klasörünün altındaki autofill-profile.json dosyasının yolu döndürülür
    }

    // Bu fonksiyon, kartların saklandığı dosyanın tam yolunu döndürür
    static std::wstring KartDosyaYolu()
    {
        return VeriKlasoruAl() + L"\\autofill-cards.json"; // Ana veri klasörünün altındaki autofill-cards.json dosyasının yolu döndürülür
    }

    // Bu fonksiyon, şu anki tarih ve saati, ISO biçiminde bir metin olarak döndürür
    static std::wstring SuankiZamaniAl()
    {
        SYSTEMTIME zaman;
        GetSystemTime(&zaman);
        wchar_t tamponMetin[32];
        swprintf_s(tamponMetin, L"%04d-%02d-%02dT%02d:%02d:%02dZ", zaman.wYear, zaman.wMonth, zaman.wDay, zaman.wHour, zaman.wMinute, zaman.wSecond);
        return tamponMetin;
    }

    // Bu fonksiyon, verilen düz metni, Windows'un DPAPI sistemiyle şifreleyip, sonucu Base64 ile kodlanmış bir metin olarak döndürür, PasswordVault.cpp'deki aynı işlemin birebir aynısıdır
    static std::wstring DuzMetniSifrele(const std::wstring& duzMetin)
    {
        DATA_BLOB girdi;
        girdi.pbData = reinterpret_cast<BYTE*>(const_cast<wchar_t*>(duzMetin.data()));
        girdi.cbData = static_cast<DWORD>(duzMetin.size() * sizeof(wchar_t));

        DATA_BLOB cikti = {};
        BOOL basarili = CryptProtectData(&girdi, L"TarayiciOtomatikDoldurma", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &cikti);
        if (!basarili) { return L""; }

        std::vector<uint8_t> hamVeri(cikti.pbData, cikti.pbData + cikti.cbData);
        LocalFree(cikti.pbData);
        return Base64Kodla(hamVeri);
    }

    std::wstring KartNumarasiniCoz(const std::wstring& sifreliVeri)
    {
        std::vector<uint8_t> hamVeri = Base64Coz(sifreliVeri);
        if (hamVeri.empty()) { return L""; }

        DATA_BLOB girdi;
        girdi.pbData = hamVeri.data();
        girdi.cbData = static_cast<DWORD>(hamVeri.size());

        DATA_BLOB cikti = {};
        BOOL basarili = CryptUnprotectData(&girdi, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &cikti);
        if (!basarili) { return L""; }

        std::wstring sonuc(reinterpret_cast<wchar_t*>(cikti.pbData), cikti.cbData / sizeof(wchar_t));
        LocalFree(cikti.pbData);
        return sonuc;
    }

    void ProfilKaydet(const OtomatikDoldurmaProfili& profil)
    {
        std::wstring icerik = L"{\"adSoyad\":\"" + JsonIcinKacisla(profil.adSoyad)
            + L"\",\"eposta\":\"" + JsonIcinKacisla(profil.eposta)
            + L"\",\"telefon\":\"" + JsonIcinKacisla(profil.telefon)
            + L"\",\"adres\":\"" + JsonIcinKacisla(profil.adres)
            + L"\"}";
        Utf8DosyaYaz(ProfilDosyaYolu(), icerik);
    }

    OtomatikDoldurmaProfili ProfilOku()
    {
        OtomatikDoldurmaProfili profil; // Tüm alanları boş başlar, dosya yoksa veya bozuksa bu şekilde kalır
        std::wstring icerik = Utf8DosyaOku(ProfilDosyaYolu());
        if (icerik.empty()) { return profil; }

        profil.adSoyad = JsonAlaniBul(icerik, L"adSoyad");
        profil.eposta = JsonAlaniBul(icerik, L"eposta");
        profil.telefon = JsonAlaniBul(icerik, L"telefon");
        profil.adres = JsonAlaniBul(icerik, L"adres");
        return profil;
    }

    // Bu fonksiyon, bir kart listesini, dosyaya yazılabilecek bir JSON dizisi metnine çevirir
    static std::wstring KartListesiniJsonYap(const std::vector<KayitliKart>& liste)
    {
        std::wstring sonuc = L"[";
        for (size_t i = 0; i < liste.size(); i++)
        {
            if (i > 0) { sonuc += L","; }
            sonuc += L"{\"kartSahibi\":\"" + JsonIcinKacisla(liste[i].kartSahibi) + L"\",\"kartNumarasiSifreli\":\"" + JsonIcinKacisla(liste[i].kartNumarasiSifreli) + L"\",\"sonKullanma\":\"" + JsonIcinKacisla(liste[i].sonKullanma) + L"\",\"eklenmeZamani\":\"" + JsonIcinKacisla(liste[i].eklenmeZamani) + L"\"}";
        }
        sonuc += L"]";
        return sonuc;
    }

    // Bu fonksiyon, bir JSON nesnesinin açılış süslü parantezinin konumundan başlayarak, dize içindeki parantezleri göz ardı edip gerçek kapanış parantezinin konumunu bulur
    // Alanlardan herhangi biri (örneğin kart sahibi adı) teorik olarak bir süslü parantez içerebileceği için, basit "ilk } karakterini bul" yöntemi yerine bu daha güvenli yöntem kullanılır
    static size_t KartNesneSonunuBul(const std::wstring& metin, size_t acilisKonumu)
    {
        int derinlik = 0;
        bool dizeIcindeMi = false;
        for (size_t i = acilisKonumu; i < metin.size(); i++)
        {
            wchar_t karakter = metin[i];
            if (dizeIcindeMi)
            {
                if (karakter == L'\\') { i++; }
                else if (karakter == L'"') { dizeIcindeMi = false; }
            }
            else
            {
                if (karakter == L'"') { dizeIcindeMi = true; }
                else if (karakter == L'{') { derinlik++; }
                else if (karakter == L'}')
                {
                    derinlik--;
                    if (derinlik == 0) { return i; }
                }
            }
        }
        return std::wstring::npos;
    }

    // Bu fonksiyon, dosyadan okunan bir JSON dizisi metnini, bir kart listesine çevirir
    static std::vector<KayitliKart> KartJsonDiziyiListeYap(const std::wstring& jsonMetni)
    {
        std::vector<KayitliKart> liste;
        size_t konum = 0;
        while (true)
        {
            size_t nesneBaslangici = jsonMetni.find(L'{', konum);
            if (nesneBaslangici == std::wstring::npos) { break; }
            size_t nesneBitisi = KartNesneSonunuBul(jsonMetni, nesneBaslangici);
            if (nesneBitisi == std::wstring::npos) { break; }

            std::wstring nesneMetni = jsonMetni.substr(nesneBaslangici, nesneBitisi - nesneBaslangici + 1);

            KayitliKart kayit;
            kayit.kartSahibi = JsonAlaniBul(nesneMetni, L"kartSahibi");
            kayit.kartNumarasiSifreli = JsonAlaniBul(nesneMetni, L"kartNumarasiSifreli");
            kayit.sonKullanma = JsonAlaniBul(nesneMetni, L"sonKullanma");
            kayit.eklenmeZamani = JsonAlaniBul(nesneMetni, L"eklenmeZamani");

            if (!kayit.kartSahibi.empty()) { liste.push_back(kayit); }

            konum = nesneBitisi + 1;
        }
        return liste;
    }

    static std::vector<KayitliKart> KartlarDosyadanOku()
    {
        return KartJsonDiziyiListeYap(Utf8DosyaOku(KartDosyaYolu()));
    }

    static void KartlarDosyayaYaz(const std::vector<KayitliKart>& liste)
    {
        Utf8DosyaYaz(KartDosyaYolu(), KartListesiniJsonYap(liste));
    }

    void KartEkle(const std::wstring& kartSahibi, const std::wstring& duzKartNumarasi, const std::wstring& sonKullanma)
    {
        std::vector<KayitliKart> liste = KartlarDosyadanOku();

        KayitliKart yeniKayit;
        yeniKayit.kartSahibi = kartSahibi;
        yeniKayit.kartNumarasiSifreli = DuzMetniSifrele(duzKartNumarasi); // Düz metin kart numarası, DPAPI ile şifrelenip kaydedilir, düz metin hiçbir zaman diske yazılmaz
        yeniKayit.sonKullanma = sonKullanma;
        yeniKayit.eklenmeZamani = SuankiZamaniAl();

        liste.push_back(yeniKayit);
        KartlarDosyayaYaz(liste);
    }

    void KartSil(const std::wstring& kartSahibi, const std::wstring& eklenmeZamani)
    {
        std::vector<KayitliKart> liste = KartlarDosyadanOku();
        std::vector<KayitliKart> yeniListe;
        for (const KayitliKart& kayit : liste)
        {
            if (!(kayit.kartSahibi == kartSahibi && kayit.eklenmeZamani == eklenmeZamani))
            {
                yeniListe.push_back(kayit);
            }
        }
        KartlarDosyayaYaz(yeniListe);
    }

    std::vector<KayitliKart> KartlariOku()
    {
        return KartlarDosyadanOku();
    }

    void TumVeriyiSil()
    {
        Utf8DosyaYaz(ProfilDosyaYolu(), L""); // Profil dosyası boşaltılır
        KartlarDosyayaYaz({}); // Kart dosyasının üzerine boş bir liste yazılır
    }
}
