// Bu dosya, UserScriptVault.h içinde tanımı yapılan kullanıcı betiği fonksiyonlarının gerçek çalışma mantığını içerir.
// Betikler, userscripts.json dosyasında, Storage.cpp'deki diğer kayıtlarla tutarlı basit bir JSON dizisi biçiminde tutulur.

#include "UserScriptVault.h" // Bu dosyanın, hangi fonksiyonların gövdesini yazdığını bilmesi için kendi başlık dosyası eklenir
#include "JsonYardimci.h" // Metinleri JSON içine yazma ve JSON nesnesinden alan okuma yardımcıları için eklenir
#include "VeriYolu.h" // Verilerin saklanacağı ana klasörün yolunu bulan ortak fonksiyon için eklenir
#include "Utf8Yardimci.h" // Dosyaları UTF-8 kodlamasıyla okuyup yazan ortak yardımcı fonksiyonlar için eklenir
#include <windows.h> // Zaman damgasından benzersiz kimlik üretmek için eklenir

namespace BetikKasasi // Tüm kullanıcı betiği fonksiyonları, başka isimlerle çakışmaması için bu isim alanının içine konur
{
    static std::wstring BetikDosyaYolu()
    {
        return VeriKlasoruAl() + L"\\userscripts.json";
    }

    static std::wstring ListeyiJsonYap(const std::vector<KullaniciBetigi>& liste)
    {
        std::wstring sonuc = L"[";
        for (size_t i = 0; i < liste.size(); i++)
        {
            if (i > 0) { sonuc += L","; }
            sonuc += L"{\"id\":\"" + JsonIcinKacisla(liste[i].id) + L"\",\"ad\":\"" + JsonIcinKacisla(liste[i].ad) + L"\",\"desen\":\"" + JsonIcinKacisla(liste[i].desen) + L"\",\"kod\":\"" + JsonIcinKacisla(liste[i].kod) + L"\",\"etkinMi\":\"" + (liste[i].etkinMi ? L"evet" : L"hayir") + L"\"}";
        }
        sonuc += L"]";
        return sonuc;
    }

    // Bu fonksiyon, bir JSON nesnesinin AÇILIŞ süslü parantezinin konumundan başlayarak, gerçek KAPANIŞ süslü parantezinin konumunu bulur
    // Basit "ilk } karakterini bul" yöntemi burada YETERSİZ kalır, çünkü "kod" alanı kullanıcının yazdığı serbest JavaScript metnidir ve neredeyse her gerçek betik kendi süslü parantezlerini (örneğin "function(){...}") içerir
    // Bu fonksiyon, tırnak içindeki (dize) karakterleri ve kaçışlanmış tırnakları atlayarak, sadece dize DIŞINDAKİ süslü parantezleri sayar, böylece betik kodunun içindeki parantezler nesnenin bittiğini yanlışlıkla işaret etmez
    static size_t NesneSonunuBul(const std::wstring& metin, size_t acilisKonumu)
    {
        int derinlik = 0;
        bool dizeIcindeMi = false;
        for (size_t i = acilisKonumu; i < metin.size(); i++)
        {
            wchar_t karakter = metin[i];
            if (dizeIcindeMi)
            {
                if (karakter == L'\\') { i++; } // Kaçışlanan karakter (örneğin kaçışlanmış bir tırnak), hiçbir özel anlamı olmadan atlanır
                else if (karakter == L'"') { dizeIcindeMi = false; }
            }
            else
            {
                if (karakter == L'"') { dizeIcindeMi = true; }
                else if (karakter == L'{') { derinlik++; }
                else if (karakter == L'}')
                {
                    derinlik--;
                    if (derinlik == 0) { return i; } // En dıştaki nesnenin kapanışı bulundu
                }
            }
        }
        return std::wstring::npos; // Kapanış hiç bulunamadı, metin bozuk demektir
    }

    static std::vector<KullaniciBetigi> JsonDiziyiListeYap(const std::wstring& jsonMetni)
    {
        std::vector<KullaniciBetigi> liste;
        size_t konum = 0;
        while (true)
        {
            size_t nesneBaslangici = jsonMetni.find(L'{', konum);
            if (nesneBaslangici == std::wstring::npos) { break; }
            size_t nesneBitisi = NesneSonunuBul(jsonMetni, nesneBaslangici); // Dize içindeki süslü parantezleri doğru şekilde göz ardı eden, derinlik sayan arama kullanılır
            if (nesneBitisi == std::wstring::npos) { break; }

            std::wstring nesneMetni = jsonMetni.substr(nesneBaslangici, nesneBitisi - nesneBaslangici + 1);

            KullaniciBetigi kayit;
            kayit.id = JsonAlaniBul(nesneMetni, L"id");
            kayit.ad = JsonAlaniBul(nesneMetni, L"ad");
            kayit.desen = JsonAlaniBul(nesneMetni, L"desen");
            kayit.kod = JsonAlaniBul(nesneMetni, L"kod");
            kayit.etkinMi = (JsonAlaniBul(nesneMetni, L"etkinMi") != L"hayir"); // Alan hiç yoksa (eski kayıt) veya "evet" ise etkin sayılır, sadece "hayir" ise kapalı sayılır

            if (!kayit.id.empty()) { liste.push_back(kayit); }

            konum = nesneBitisi + 1;
        }
        return liste;
    }

    static std::vector<KullaniciBetigi> DosyadanOku()
    {
        return JsonDiziyiListeYap(Utf8DosyaOku(BetikDosyaYolu()));
    }

    static void DosyayaYaz(const std::vector<KullaniciBetigi>& liste)
    {
        Utf8DosyaYaz(BetikDosyaYolu(), ListeyiJsonYap(liste));
    }

    void BetikEkle(const std::wstring& ad, const std::wstring& desen, const std::wstring& kod)
    {
        std::vector<KullaniciBetigi> liste = DosyadanOku();

        KullaniciBetigi yeniKayit;
        FILETIME simdikiZaman;
        GetSystemTimeAsFileTime(&simdikiZaman); // Zaman damgası, her betik için benzersiz bir kimlik üretmekte kullanılır
        yeniKayit.id = std::to_wstring(simdikiZaman.dwLowDateTime) + std::to_wstring(simdikiZaman.dwHighDateTime);
        yeniKayit.ad = ad;
        yeniKayit.desen = desen;
        yeniKayit.kod = kod;
        yeniKayit.etkinMi = true;

        liste.push_back(yeniKayit);
        DosyayaYaz(liste);
    }

    void BetikSil(const std::wstring& id)
    {
        std::vector<KullaniciBetigi> liste = DosyadanOku();
        std::vector<KullaniciBetigi> yeniListe;
        for (const KullaniciBetigi& kayit : liste)
        {
            if (kayit.id != id) { yeniListe.push_back(kayit); }
        }
        DosyayaYaz(yeniListe);
    }

    void BetikEtkinligiDegistir(const std::wstring& id, bool etkinMi)
    {
        std::vector<KullaniciBetigi> liste = DosyadanOku();
        for (KullaniciBetigi& kayit : liste)
        {
            if (kayit.id == id) { kayit.etkinMi = etkinMi; break; }
        }
        DosyayaYaz(liste);
    }

    std::vector<KullaniciBetigi> BetikleriOku()
    {
        return DosyadanOku();
    }
}
