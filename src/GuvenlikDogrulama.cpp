// Bu dosya, GuvenlikDogrulama.h içinde tanımı yapılan Windows şifre doğrulama penceresinin gerçek çalışma mantığını içerir.
// Pencere, Win32 kontrolleriyle (etiket, şifre kutusu, iki buton) elle oluşturulur, bir kaynak (.rc) dosyasına ihtiyaç yoktur.
// Girilen şifrenin doğru olup olmadığı, Windows'un kendi LogonUserW fonksiyonuna sorularak anlaşılır, bu fonksiyon şifreyi bizim tarafımızdan hiçbir yerde saklanmadan doğrudan Windows'a doğrulatır.

#include "GuvenlikDogrulama.h" // Bu dosyanın, hangi fonksiyonun gövdesini yazdığını bilmesi için kendi başlık dosyası eklenir
#include <string> // Metin (std::wstring) türünü kullanmak için eklenir

namespace Guvenlik // GuvenlikDogrulama.h'deki isim alanıyla aynı isim alanı burada da kullanılır
{
    // Bu sabitler, pencere içindeki kontrollerin birbirinden ayırt edilebilmesi için her kontrole verilen benzersiz numaralardır
    static const int KontrolIdSifreKutusu = 101;
    static const int KontrolIdOnaylaButonu = 102;
    static const int KontrolIdIptalButonu = 103;

    // Bu yapı, pencere açıkken ihtiyaç duyulan geçici bilgileri (şifre kutusunun tanıtıcısı ve sonucun ne olduğu) bir arada tutar
    struct PencereDurumu
    {
        HWND sifreKutusu = nullptr; // Kullanıcının şifre yazdığı metin kutusunun tanıtıcısı
        bool sonuc = false; // Doğrulamanın başarılı olup olmadığı, pencere kapanınca bu alan okunur
        bool kapandi = false; // Pencerenin kapanıp kapanmadığı, mesaj döngüsünün ne zaman duracağını bilmek için kullanılır
    };

    // Bu fonksiyon, kullanıcının girdiği şifrenin, şu anda Windows'ta oturum açmış kullanıcının gerçek şifresiyle eşleşip eşleşmediğini kontrol eder
    static bool GercekSifreyleKarsilastir(const std::wstring& girilenSifre)
    {
        wchar_t kullaniciAdi[256]; // Şu anda oturum açmış kullanıcının adını tutacak tampon
        DWORD kullaniciAdiUzunlugu = 256; // Tamponun boyutu, GetUserNameW fonksiyonuna bildirilir
        GetUserNameW(kullaniciAdi, &kullaniciAdiUzunlugu); // Şu anda oturum açmış Windows kullanıcısının adı okunur

        HANDLE belirtec = nullptr; // Doğrulama başarılı olursa Windows'un döndüreceği oturum belirtecini tutacak değişken
        // LOGON32_LOGON_INTERACTIVE ile, tıpkı kullanıcı bilgisayarı açtığında şifresini yazması gibi bir doğrulama istenir, "." bu bilgisayarın yerel hesabı anlamına gelir
        BOOL basarili = LogonUserW(kullaniciAdi, L".", girilenSifre.c_str(), LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &belirtec);

        if (basarili && belirtec != nullptr) // Eğer doğrulama başarılıysa
        {
            CloseHandle(belirtec); // Alınan oturum belirteci artık kullanılmayacağı için kapatılır
            return true; // Şifrenin doğru olduğu bildirilir
        }

        return false; // Doğrulama başarısızsa, şifrenin yanlış olduğu bildirilir
    }

    // Bu fonksiyon, güvenlik doğrulama penceresine gelen her mesajı işler
    static LRESULT CALLBACK PencereMesajIsleyici(HWND pencere, UINT mesaj, WPARAM birinciParametre, LPARAM ikinciParametre)
    {
        PencereDurumu* durum = reinterpret_cast<PencereDurumu*>(GetWindowLongPtr(pencere, GWLP_USERDATA)); // Bu pencereye ait durum bilgisine erişilir

        if (mesaj == WM_CREATE) // Pencere ilk oluşturulduğu anda
        {
            CREATESTRUCT* olusturmaBilgisi = reinterpret_cast<CREATESTRUCT*>(ikinciParametre); // Pencere oluşturulurken verilen bilgilere erişilir
            durum = reinterpret_cast<PencereDurumu*>(olusturmaBilgisi->lpCreateParams); // Dışarıdan gönderilen durum nesnesinin adresi alınır
            SetWindowLongPtr(pencere, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(durum)); // Bu adres, pencerenin özel veri alanına kaydedilir

            HINSTANCE uygulamaOrnegi = olusturmaBilgisi->hInstance; // Pencerenin ait olduğu uygulama örneği alınır

            CreateWindowW(L"STATIC", L"Devam etmek için Windows şifrenizi girin.", WS_CHILD | WS_VISIBLE,
                20, 20, 320, 20, pencere, nullptr, uygulamaOrnegi, nullptr); // Açıklama metnini gösteren sabit bir etiket oluşturulur

            durum->sifreKutusu = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
                20, 50, 320, 24, pencere, reinterpret_cast<HMENU>(static_cast<INT_PTR>(KontrolIdSifreKutusu)), uygulamaOrnegi, nullptr); // Kullanıcının şifresini yazacağı, yazdığı karakterleri gizleyen bir metin kutusu oluşturulur

            CreateWindowW(L"BUTTON", L"Onayla", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                180, 90, 80, 28, pencere, reinterpret_cast<HMENU>(static_cast<INT_PTR>(KontrolIdOnaylaButonu)), uygulamaOrnegi, nullptr); // Onaylama butonu oluşturulur

            CreateWindowW(L"BUTTON", L"İptal", WS_CHILD | WS_VISIBLE,
                270, 90, 70, 28, pencere, reinterpret_cast<HMENU>(static_cast<INT_PTR>(KontrolIdIptalButonu)), uygulamaOrnegi, nullptr); // İptal butonu oluşturulur

            SetFocus(durum->sifreKutusu); // Pencere açılır açılmaz, kullanıcı doğrudan yazmaya başlayabilsin diye şifre kutusuna odaklanılır
            return 0; // Mesaj işlendi
        }

        if (mesaj == WM_COMMAND) // Bir kontrolden (buton veya metin kutusu) bir bildirim geldiğinde
        {
            int kontrolId = LOWORD(birinciParametre); // Hangi kontrolden geldiği, mesajın alt on altı bitinden okunur

            if (kontrolId == KontrolIdOnaylaButonu) // Eğer Onayla butonuna tıklandıysa
            {
                wchar_t girilenMetin[256]; // Şifre kutusundaki metni okumak için bir tampon
                GetWindowTextW(durum->sifreKutusu, girilenMetin, 256); // Kullanıcının yazdığı şifre okunur

                if (GercekSifreyleKarsilastir(girilenMetin)) // Eğer girilen şifre, Windows'un kendi doğrulamasından geçtiyse
                {
                    durum->sonuc = true; // Doğrulamanın başarılı olduğu kaydedilir
                    DestroyWindow(pencere); // Pencere kapatılır
                }
                else // Eğer şifre yanlışsa
                {
                    MessageBoxW(pencere, L"Girilen şifre yanlış, lütfen tekrar deneyin.", L"Doğrulama Başarısız", MB_OK | MB_ICONWARNING); // Kullanıcıya şifrenin yanlış olduğu bildirilir
                    SetWindowTextW(durum->sifreKutusu, L""); // Şifre kutusu temizlenir, böylece kullanıcı tekrar yazabilir
                    SetFocus(durum->sifreKutusu); // Şifre kutusuna tekrar odaklanılır
                }
                return 0; // Mesaj işlendi
            }

            if (kontrolId == KontrolIdIptalButonu) // Eğer İptal butonuna tıklandıysa
            {
                durum->sonuc = false; // Doğrulamanın başarısız (iptal edilmiş) olduğu kaydedilir
                DestroyWindow(pencere); // Pencere kapatılır
                return 0; // Mesaj işlendi
            }
        }

        if (mesaj == WM_CLOSE) // Kullanıcı pencereyi kapatma işaretine (sağ üstteki çarpıya) tıkladıysa
        {
            durum->sonuc = false; // Doğrulamanın başarısız (iptal edilmiş) olduğu kaydedilir
            DestroyWindow(pencere); // Pencere kapatılır
            return 0; // Mesaj işlendi
        }

        if (mesaj == WM_DESTROY) // Pencere kapatılıyorken
        {
            durum->kapandi = true; // Mesaj döngüsünün durması gerektiği işaretlenir
            return 0; // Mesaj işlendi
        }

        return DefWindowProc(pencere, mesaj, birinciParametre, ikinciParametre); // Diğer tüm mesajlar, Windows'un varsayılan işleyicisine bırakılır
    }

    // Bu fonksiyon, kullanıcıya Windows şifresini soran küçük bir pencere açar, pencere kapanana kadar bekler, sonucu döndürür
    bool WindowsSifresiDogrula(HWND sahipPencere)
    {
        HINSTANCE uygulamaOrnegi = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(sahipPencere, GWLP_HINSTANCE)); // Ana pencerenin ait olduğu uygulama örneği alınır

        const wchar_t* sinifAdi = L"TarayiciGuvenlikPenceresi"; // Bu doğrulama penceresine özel sınıf ismi
        static bool sinifKayitliMi = false; // Pencere sınıfının daha önce kaydedilip kaydedilmediğini hatırlayan değişken, her çağrıda tekrar kaydetmemek için kullanılır

        if (!sinifKayitliMi) // Eğer sınıf daha önce kaydedilmediyse
        {
            WNDCLASS sinif = {}; // Pencere sınıfı ayarlarını tutacak boş bir yapı oluşturulur
            sinif.lpfnWndProc = PencereMesajIsleyici; // Bu sınıftan oluşan pencerelere gelen mesajların işleyicisi belirtilir
            sinif.hInstance = uygulamaOrnegi; // Sınıfın ait olduğu uygulama örneği belirtilir
            sinif.lpszClassName = sinifAdi; // Sınıfın ismi atanır
            sinif.hCursor = LoadCursor(nullptr, IDC_ARROW); // Fare imlecinin normal ok şeklinde görünmesi sağlanır
            sinif.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1); // Pencerenin arka planı, Windows'un standart pencere rengiyle doldurulur
            RegisterClass(&sinif); // Pencere sınıfı, Windows işletim sistemine kaydedilir
            sinifKayitliMi = true; // Sınıfın artık kayıtlı olduğu işaretlenir
        }

        RECT sahipAlani; // Sahip pencerenin ekrandaki konumunu tutacak dikdörtgen
        GetWindowRect(sahipPencere, &sahipAlani); // Sahip pencerenin konumu okunur, doğrulama penceresini onun ortasına yerleştirmek için kullanılır

        int genislik = 380; // Doğrulama penceresinin genişliği
        int yukseklik = 170; // Doğrulama penceresinin yüksekliği
        int x = sahipAlani.left + ((sahipAlani.right - sahipAlani.left) - genislik) / 2; // Pencerenin yatayda sahip penceresinin tam ortasına gelecek konumu hesaplanır
        int y = sahipAlani.top + ((sahipAlani.bottom - sahipAlani.top) - yukseklik) / 2; // Pencerenin dikeyde sahip penceresinin tam ortasına gelecek konumu hesaplanır

        PencereDurumu durum; // Bu doğrulama işlemine özel geçici durum nesnesi oluşturulur

        EnableWindow(sahipPencere, FALSE); // Doğrulama penceresi açıkken, ana pencereyle etkileşime girilmesi engellenir, böylece pencere gerçekten kipli (modal) davranır

        HWND dogrulamaPenceresi = CreateWindowExW(
            WS_EX_DLGMODALFRAME, sinifAdi, L"Kimlik Doğrulama", WS_POPUP | WS_CAPTION | WS_SYSMENU,
            x, y, genislik, yukseklik, sahipPencere, nullptr, uygulamaOrnegi, &durum
        ); // Doğrulama penceresi, sahip pencerenin ortasında, sabit boyutlu bir açılır pencere olarak oluşturulur

        ShowWindow(dogrulamaPenceresi, SW_SHOW); // Pencere ekranda görünür hale getirilir

        MSG mesaj; // Bu pencereye özel küçük mesaj döngüsünde kullanılacak mesaj yapısı
        while (!durum.kapandi && GetMessage(&mesaj, nullptr, 0, 0)) // Pencere kapanana kadar, gelen her mesaj işlenir
        {
            if (!IsDialogMessage(dogrulamaPenceresi, &mesaj)) // Sekme tuşuyla kontroller arası geçiş gibi standart pencere davranışları varsayılan olarak ele alınır
            {
                TranslateMessage(&mesaj); // Klavye tuş mesajları daha kullanılabilir bir hale çevrilir
                DispatchMessage(&mesaj); // Mesaj, ilgili pencerenin mesaj işleyicisine gönderilir
            }
        }

        EnableWindow(sahipPencere, TRUE); // Ana pencereyle etkileşim tekrar etkinleştirilir
        SetForegroundWindow(sahipPencere); // Ana pencere tekrar öne getirilir

        return durum.sonuc; // Doğrulamanın başarılı olup olmadığı döndürülür
    }

    // Bu değişken, kullanıcının en son BAŞARIYLA doğrulandığı anı (GetTickCount64 değeri olarak) tutar, program içi bir "kasa en son ne zaman açıldı" hafızasıdır
    // Bu bilgi diske hiç yazılmaz, sadece program çalışırken bellekte tutulur, program yeniden başlatıldığında (veya bilgisayar başka bir kullanıcıya geçtiğinde) her zaman sıfırdan başlar
    static ULONGLONG g_sonBasariliDogrulamaZamani = 0;

    bool WindowsSifresiniKilitlenmeSuresiyleDogrula(HWND sahipPencere, int kilitlenmeDakikasi)
    {
        if (kilitlenmeDakikasi > 0 && g_sonBasariliDogrulamaZamani != 0) // Kullanıcı bu kolaylığı açtıysa (sıfırdan büyükse) ve daha önce en az bir kez doğrulandıysa
        {
            ULONGLONG gecenSureMs = GetTickCount64() - g_sonBasariliDogrulamaZamani; // Son başarılı doğrulamadan bu yana geçen süre, milisaniye cinsinden hesaplanır
            if (gecenSureMs < static_cast<ULONGLONG>(kilitlenmeDakikasi) * 60ULL * 1000ULL) // Eğer bu süre, kullanıcının seçtiği kilitlenme süresinden daha azsa, "kasa" hâlâ açık sayılır
            {
                return true; // Tekrar sormadan doğrudan doğrulanmış kabul edilir
            }
        }

        bool sonuc = WindowsSifresiDogrula(sahipPencere); // Kasa kilitliyse (veya kolaylık kapalıysa), kullanıcıya gerçekten sorulur
        if (sonuc) // Doğrulama başarılıysa
        {
            g_sonBasariliDogrulamaZamani = GetTickCount64(); // Bu an, bir sonraki çağrı için hatırlanır, "kasa" bu andan itibaren kilitlenme süresi boyunca açık sayılır
        }
        return sonuc;
    }
}
