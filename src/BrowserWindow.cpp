// Bu dosya, BrowserWindow.h içinde tanımı yapılan BrowserWindow sınıfının gerçek çalışma mantığını içerir.
// Burada, Win32 penceresinin nasıl oluşturulduğu, toolbar WebView2'sinin ve birden fazla sekmenin nasıl kurulduğu, sekmeler arasında nasıl geçiş yapıldığı, geçmiş/yer imi/ayar/şifre kayıtlarının ne zaman tutulduğu, dahili Ayarlar sekmesinin nasıl çalıştığı, indirmelerin nasıl onaylandığı, uzun süre kullanılmayan sekmelerin nasıl uykuya alındığı, ve bir sayfa açılamadığında kullanıcıya nasıl kodlu bir hata ekranı gösterildiği yazılıdır.
// Bu dosya, WebView2 kütüphanesi ile doğrudan konuşan tek dosyadır, geçmiş/yer imi için Storage.h, hata kaydı için Logger.h, ayarlar için Settings.h, şifreler için PasswordVault.h ve GuvenlikDogrulama.h dosyalarını kullanır.

#include "BrowserWindow.h" // Bu dosyanın, hangi sınıfın gövdesini yazdığını bilmesi için kendi başlık dosyası eklenir
#include "resource.h" // Programın gömülü simgesinin (IDI_APPICON) kimlik numarasını kullanmak için eklenir
#include "Storage.h" // Geçmişe kayıt ekleme ve yer imi işlemleri için eklenir
#include "Logger.h" // Hata kodlarını günlüğe kaydetme, kodun açıklamasını bulma ve günlüğü okuma için eklenir
#include "Settings.h" // Kullanıcının arama motoru, ana sayfa ve tema ayarlarını okuyup yazmak için eklenir
#include "ProgramSurumu.h" // Çalışan .exe'nin kendi gömülü sürümünü okuyup, kullanıcının rıza onayı verdiği sürümle karşılaştırmak için eklenir
#include "PasswordVault.h" // Kayıtlı şifreleri eklemek, silmek, listelemek ve çözmek için eklenir
#include "AutofillVault.h" // Kişisel bilgi profilini ve kayıtlı kartları eklemek, silmek, listelemek ve çözmek için eklenir
#include "UserScriptVault.h" // Kullanıcının kendi eklediği küçük JavaScript betiklerini (userscript) eklemek, silmek, listelemek için eklenir
#include "PermissionVault.h" // Kullanıcının sitelere verdiği izin kararlarını (konum, kamera, mikrofon gibi) kaydetmek ve okumak için eklenir
#include "Engelleyici.h" // Reklam ve izleyici (tracker) engelleme için küratörlü domain listelerini kontrol eden fonksiyonlar için eklenir
#include "VeriYolu.h" // Mağazadan indirilen uzantıların paketlerinin açılacağı klasörün yolunu bulmak için eklenir
#include "GuvenlikDogrulama.h" // Bir şifreyi göstermeden önce Windows şifresini doğrulamak için eklenir
#include "JsonYardimci.h" // Toolbar ve Ayarlar sekmesine gönderilen metinleri JSON içine güvenle yazmak ve JSON'dan alan okumak için eklenir
#include "Base64Yardimci.h" // Sekme önizleme görüntüsünün ham PNG baytlarını, JSON içinde taşınabilecek bir metne çevirmek için eklenir
#include "Utf8Yardimci.h" // DevTools hedef listesini sunan yerel sunucudan gelen UTF-8 yanıtı, geniş metne çevirmek için eklenir
#include <WebView2EnvironmentOptions.h> // WebView2 ortamını, uzaktan hata ayıklama portu gibi ekstra başlangıç parametreleriyle oluşturmak için eklenir
#include <winhttp.h> // Gömülü DevTools panelinin, hangi sekmeye bağlanacağını bulmak için yerel hata ayıklama sunucusuna HTTP isteği atmak için eklenir
#include <thread> // DevTools hedef bulma isteğinin, arayüzü bloke etmeden ayrı bir iş parçacığında çalıştırılması için eklenir
#include <tlhelp32.h> // Çalışan süreçlerin listesini almak (ToplamRamKullanimiMB için) için eklenir
#include <psapi.h> // Bir sürecin bellek kullanım bilgisini almak için eklenir
#include <set> // RAM hesaplanırken ziyaret edilen süreç kimliklerini tutmak için eklenir
#include <map> // En çok ziyaret edilen siteleri hesaplarken adres başına sayaç tutmak için eklenir
#include <algorithm> // En çok ziyaret edilen siteleri sayılarına göre sıralamak için eklenir
#include <ctime> // Veri temizleme panelindeki zaman aralığı hesaplamaları (time_t, gmtime_s) için eklenir
#include <combaseapi.h> // Sekme önizlemesini belleğe yakalamak için kullanılan CreateStreamOnHGlobal fonksiyonu için eklenir
#include <oleauto.h> // Sertifika geçerlilik tarihlerini (OLE otomasyon tarihi biçiminde gelir) okunabilir bir tarihe çeviren VariantTimeToSystemTime fonksiyonu için eklenir
#include <shellapi.h> // İndirme tamamlanma bildirimini bildirim alanında göstermek (Shell_NotifyIcon) ve "Klasörde Göster" özelliğini (ShellExecuteW) kullanmak için eklenir
#include <commdlg.h> // Kullanıcının özel yeni sekme arka planı için bir resim seçebileceği standart "Dosya Aç" penceresini (GetOpenFileNameW) açmak için eklenir
#include <shlobj.h> // Kullanıcının paketlenmemiş bir uzantı klasörü seçebileceği standart "Klasör Seç" penceresini (SHBrowseForFolderW) açmak için eklenir
#include <fstream> // Seçilen arka plan resminin ham baytlarını diskten okumak için eklenir
#include <cstdint> // uint32_t/uint8_t türlerini kullanmak için eklenir, CRX paket başlığını ayrıştırırken kullanılır
#include <wincodec.h> // Arka plan galerisindeki fotoğrafların küçük (128x128) önizlemelerini, Base64'e çevirip JSON'a gömmek yerine WIC ile anlık olarak küçültüp diske yazmak için eklenir, tam boyutlu fotoğrafları önizlemede göstermek arayüzü ciddi şekilde kasıyordu

using namespace Microsoft::WRL; // ComPtr ve Callback gibi isimleri her yerde Microsoft::WRL:: önekiyle yazmamak için bu isim alanı kullanılır

// İleri bildirim: bu fonksiyonun gövdesi dosyanın ilerisinde tanımlıdır, ama pencere mesaj işleyicisi (WndProc) dosyanın başında olduğu ve onu kullandığı için önce bildirilmesi gerekir
static ComPtr<ICoreWebView2Profile7> EklentiProfiliniAl(ICoreWebView2* herhangiBirWebView);

// İleri bildirim: "resimler" klasörünün tam yolunu bulan fonksiyon, dosyanın ilerisinde tanımlıdır ama sekme oluşturma kodu (dahili sayfalara sanal klasör eşleme) bu fonksiyonu daha yukarıda kullanır
static std::wstring ResimlerKlasorunuBul();
static std::vector<std::wstring> ResimlerKlasorunuTara(const std::wstring& klasor);

// Bu fonksiyon, çalışmakta olan programın (tarayici.exe dosyasının) bulunduğu klasörün tam yolunu bulur, böylece assets klasörüne doğru şekilde ulaşılabilir
static std::wstring CalismaKlasoruBul()
{
    wchar_t yol[MAX_PATH]; // Programın tam dosya yolunu tutacak geçici bir metin dizisi oluşturulur
    GetModuleFileNameW(nullptr, yol, MAX_PATH); // Çalışmakta olan programın kendi tam dosya yolu bu diziye yazılır
    std::wstring tamYol(yol); // Elde edilen diziyi daha kolay işlemek için bir std::wstring'e çevirir
    size_t sonTersBolu = tamYol.find_last_of(L'\\'); // Dosya yolundaki en sondaki ters bölü işaretinin konumu bulunur
    return tamYol.substr(0, sonTersBolu); // Dosya adını atıp, sadece programın içinde bulunduğu klasörün yolu döndürülür
}

// Bu fonksiyon, adres çubuğuna veya arama kutusuna yazılacak bir metni, bir web adresinin içine güvenli şekilde koyabilmek için kodlar
static std::wstring MetniAdresIcinKodla(const std::wstring& metin)
{
    // Önce metin UTF-8 byte dizisine çevrilir, çünkü yüzde kodlaması UTF-16 kod noktası üzerinden değil UTF-8 byte'ları üzerinden yapılmalıdır (aksi halde ğ, ş, ı gibi harfler bozulur)
    int gerekliBoyut = WideCharToMultiByte(CP_UTF8, 0, metin.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8Metin(gerekliBoyut > 0 ? gerekliBoyut - 1 : 0, '\0'); // Sondaki null karakter hariç tutulur
    if (gerekliBoyut > 0)
    {
        WideCharToMultiByte(CP_UTF8, 0, metin.c_str(), -1, utf8Metin.data(), gerekliBoyut, nullptr, nullptr);
    }

    std::wstring sonuc; // Kodlanmış sonucu biriktirecek boş bir metin oluşturulur
    for (unsigned char bayt : utf8Metin) // UTF-8 byte dizisindeki her byte sırayla gezilir
    {
        if ((bayt >= 'a' && bayt <= 'z') || (bayt >= 'A' && bayt <= 'Z') || (bayt >= '0' && bayt <= '9') || bayt == '-' || bayt == '_' || bayt == '.')
        {
            sonuc += static_cast<wchar_t>(bayt); // Harf, rakam veya zararsız bir işaretse, olduğu gibi bırakılır
        }
        else if (bayt == ' ') // Eğer karakter bir boşluksa
        {
            sonuc += L"%20"; // Web adreslerinde boşluk yüzde yirmi olarak yazılır
        }
        else // Diğer tüm özel byte'lar için
        {
            wchar_t tamponMetin[8]; // Byte'ın yüzde kodlu halini yazmak için küçük bir tampon oluşturulur
            swprintf_s(tamponMetin, L"%%%02X", static_cast<unsigned int>(bayt)); // Byte'ın sayısal değeri, iki haneli onaltılık taban olarak yazılır
            sonuc += tamponMetin; // Kodlanmış byte, sonuca eklenir
        }
    }
    return sonuc; // Tamamen kodlanmış metin döndürülür
}

// Bu fonksiyon, bir kayıt listesini (SayfaKaydi), toolbar/ayarlar arayüzüne gönderilecek bir JSON dizisi metnine çevirir
static std::wstring SayfaKayitlariniJsonYap(const std::vector<SayfaKaydi>& liste)
{
    std::wstring sonuc = L"["; // JSON dizisinin açılış köşeli parantezi ile başlanır
    for (size_t i = 0; i < liste.size(); i++) // Listedeki her kayıt sırayla gezilir
    {
        if (i > 0) { sonuc += L","; } // İlk kayıttan sonraki her kayıttan önce virgül eklenir
        sonuc += L"{\"adres\":\"" + JsonIcinKacisla(liste[i].adres) + L"\",\"baslik\":\"" + JsonIcinKacisla(liste[i].baslik) + L"\",\"zaman\":\"" + JsonIcinKacisla(liste[i].zaman) + L"\",\"klasor\":\"" + JsonIcinKacisla(liste[i].klasor) + L"\"}"; // Her kayıt bir JSON nesnesi olarak yazılır
    }
    sonuc += L"]"; // JSON dizisinin kapanış köşeli parantezi eklenir
    return sonuc; // Hazırlanan JSON dizisi metni döndürülür
}

// Bu fonksiyon, verilen bir web adresinden, sadece alan adı (host) kısmını ayıklar, örneğin "https://www.example.com/sayfa" adresinden "www.example.com" döner
static std::wstring HostCikar(const std::wstring& adres)
{
    size_t semaBasi = adres.find(L"://"); // Adresin "://" kısmının konumu aranır
    if (semaBasi == std::wstring::npos) // Eğer bu kalıp bulunamadıysa, adres beklenen biçimde değildir
    {
        return adres; // Adresin tamamı olduğu gibi döndürülür
    }
    size_t hostBasi = semaBasi + 3; // Alan adının başladığı konum, "://" kalıbının hemen ötesidir
    size_t hostSonu = adres.find(L'/', hostBasi); // Alan adının bittiği, yolun başladığı ilk ters olmayan bölü işareti aranır
    if (hostSonu == std::wstring::npos) // Eğer adreste hiç yol kısmı yoksa
    {
        hostSonu = adres.size(); // Alan adı, adresin sonuna kadar sürer
    }
    return adres.substr(hostBasi, hostSonu - hostBasi); // Sadece alan adı kısmı döndürülür
}

// Bu fonksiyon, geçmiş kayıtlarını alan adına göre gruplayıp sayar, en çok ziyaret edilen "azami" sayıda alan adını, hızlı erişim sayfasında gösterilecek kayıtlar olarak döndürür
static std::vector<SayfaKaydi> EnCokZiyaretEdilenler(int azami)
{
    std::vector<SayfaKaydi> gecmis = Depolama::GecmisiOku(); // Tüm geçmiş kayıtları okunur

    struct SayacBilgisi { int sayac = 0; std::wstring adres; std::wstring gosterimAdi; }; // Her alan adı için, kaç kere ziyaret edildiği, örnek bir adresi, ve "www." öneki olmadan gösterim ismi tutulur
    std::map<std::wstring, SayacBilgisi> sayaclar; // Alan adından sayaç bilgisine eşlenen harita

    for (const SayfaKaydi& kayit : gecmis) // Geçmişteki her kayıt sırayla gezilir
    {
        std::wstring host = HostCikar(kayit.adres); // Bu kaydın alan adı ayıklanır
        std::wstring gosterimHostu = host; // Kullanıcıya gösterilecek, sadeleştirilmiş alan adı
        if (gosterimHostu.rfind(L"www.", 0) == 0) // Eğer alan adı "www." ile başlıyorsa
        {
            gosterimHostu = gosterimHostu.substr(4); // Bu önek kaldırılır, böylece hem ilk harf doğru site adını yansıtır hem de görünüm sadeleşir
        }
        sayaclar[host].sayac++; // Bu alan adının sayacı bir artırılır (gerçek alan adıyla sayılır, böylece www'li ve www'siz aynı site karışmaz)
        sayaclar[host].adres = L"https://" + host + L"/"; // Tıklanınca gidilecek adres, gerçek alan adıyla hazırlanır
        sayaclar[host].gosterimAdi = gosterimHostu; // Kullanıcıya gösterilecek isim, "www." öneki olmadan hazırlanır
    }

    std::vector<std::pair<std::wstring, SayacBilgisi>> siraliListe(sayaclar.begin(), sayaclar.end()); // Harita, sıralanabilmesi için bir listeye çevrilir
    std::sort(siraliListe.begin(), siraliListe.end(), [](const auto& a, const auto& b) { return a.second.sayac > b.second.sayac; }); // Liste, en çok ziyaret edilenden en aza doğru sıralanır

    std::vector<SayfaKaydi> sonuc; // Döndürülecek sonuç listesi
    for (size_t i = 0; i < siraliListe.size() && static_cast<int>(i) < azami; i++) // En baştaki, en çok ziyaret edilen "azami" kadar alan adı alınır
    {
        SayfaKaydi kayit; // Hızlı erişim listesi için bir kayıt hazırlanır
        kayit.adres = siraliListe[i].second.adres; // Tıklanınca gidilecek adres atanır
        kayit.baslik = siraliListe[i].second.gosterimAdi; // "www." öneki kaldırılmış, sadeleştirilmiş alan adı, başlık olarak kullanılır
        kayit.zaman = std::to_wstring(siraliListe[i].second.sayac); // Ziyaret sayısı, zaman alanının içine geçici olarak yazılır, hızlı erişim sayfası bunu isterse gösterebilir
        sonuc.push_back(kayit); // Kayıt, sonuç listesine eklenir
    }
    return sonuc; // Hesaplanan en çok ziyaret edilenler listesi döndürülür
}

// Bu fonksiyon, güncel tema ayarlarını (vurgu rengi, sekme yoğunluğu, yazı boyutu, köşe yuvarlaklığı, açık/koyu mod), tek bir JSON nesnesi metni olarak hazırlar
// Bu fonksiyon, hem toolbar'a hem dahili sayfalara hem de Ayarlar sayfasının tam ayarlar mesajına gömülü olarak gönderilen tema bilgisini TEK bir yerde ürettiği için, yeni bir tema alanı eklendiğinde sadece burası güncellenmesi yeterlidir
// Tüm alanlar, tıpkı fontSize gibi, kod adı (örneğin "kesik"/"orta"/"yuvarlak") olarak gönderilir, bu kod adının gerçek piksel değerine çevrilmesi JavaScript tarafında yapılır, native taraf sadece kullanıcının seçtiği ayarı taşır
static std::wstring TemaNesnesiJsonYap(const AyarVerisi& a)
{
    return L"{\"accent\":\"" + JsonIcinKacisla(a.vurguRengi)
        + L"\",\"density\":\"" + JsonIcinKacisla(a.sekmeYogunlugu)
        + L"\",\"fontSize\":\"" + JsonIcinKacisla(a.yaziBoyutu)
        + L"\",\"radius\":\"" + JsonIcinKacisla(a.koseYuvarlakligi)
        + L"\",\"mode\":\"" + JsonIcinKacisla(a.temaModu)
        + L"\",\"showBookmarkBar\":" + (a.yerImiCubuguGoster ? L"true" : L"false")
        + L",\"energySaver\":" + (a.enerjiTasarrufuModu ? L"true" : L"false")
        + L"}";
}

// Bu fonksiyon, programın en başında bir kere çağrılır, hem gerçek Win32 penceresini açar hem de WebView2 ortamının kurulmasını başlatır
bool BrowserWindow::Create(HINSTANCE uygulamaOrnegi)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); // WebView2'nin arka planda kullandığı COM alt yapısını bu iş parçacığında kullanıma hazırlar

    const wchar_t* pencereSinifAdi = L"TarayiciPencereSinifi"; // Pencere sınıfımıza verilen benzersiz isim

    WNDCLASS pencereSinifi = {}; // Pencere sınıfı ayarlarını tutacak boş bir yapı oluşturulur
    pencereSinifi.lpfnWndProc = StatikPencereMesajIsleyici; // Bu sınıftan oluşan pencerelere gelen mesajların, bu sınıfın statik işleyicisine gideceği belirtilir
    pencereSinifi.hInstance = uygulamaOrnegi; // Pencere sınıfının ait olduğu uygulama örneği belirtilir
    pencereSinifi.lpszClassName = pencereSinifAdi; // Pencere sınıfının ismi atanır
    pencereSinifi.hCursor = LoadCursor(nullptr, IDC_ARROW); // Fare imlecinin normal ok şeklinde görünmesi sağlanır
    const int buyukIkonBoyutu = GetSystemMetrics(SM_CXICON); // Görev çubuğu ve Alt+Tab'da kullanılan büyük simge, sistemin varsayılan boyutunda yüklenir (asıl sorun simgenin içindeki boşluktu, app.ico artık kenarlara kadar dolu)
    const int kucukIkonBoyutu = GetSystemMetrics(SM_CXSMICON); // Pencere başlık çubuğu/sistem menüsünde kullanılan küçük simge de varsayılan boyutta yüklenir
    HICON buyukIkon = (HICON)LoadImage(uygulamaOrnegi, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, buyukIkonBoyutu, buyukIkonBoyutu, 0); // Pencerenin başlık çubuğunda ve görev çubuğunda gösterilecek gömülü simge
    HICON kucukIkon = (HICON)LoadImage(uygulamaOrnegi, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, kucukIkonBoyutu, kucukIkonBoyutu, 0); // Küçük simge ayrıca belirtilmezse Windows büyük simgeyi küçülterek kullanır, bu da bulanık görünüme yol açar
    pencereSinifi.hIcon = buyukIkon; // WNDCLASS yapısında sadece büyük simge alanı bulunur, küçük simge aşağıda WM_SETICON ile pencereye ayrıca bildirilir
    RegisterClass(&pencereSinifi); // Pencere sınıfı, Windows işletim sistemine kaydedilir

    // Pencere, WS_CAPTION (Windows'un başlık çubuğu) stilini BİLEREK İÇERİR, çünkü Windows'un küçültme/büyütme/geri yükleme sırasındaki akıcı animasyonları, ancak pencere bu stile sahipse çalışır
    // Başlık çubuğunun kendisi görsel olarak hiç çizilmeyecek, bunu PencereMesajIsle içindeki WM_NCCALCSIZE işleyicisi sağlıyor, ama pencere Windows'a "normal, animasyonlu bir pencereyim" demeye devam ediyor
    m_pencere = CreateWindowEx(
        0, pencereSinifAdi, L"Yuinn", WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 800,
        nullptr, nullptr, uygulamaOrnegi, this // Bu BrowserWindow nesnesinin kendi adresi, pencereye özel veri olarak gönderilir
    );

    if (m_pencere == nullptr) // Eğer pencere oluşturulamadıysa
    {
        return false; // Başarısızlık bildirilir, program devam edemez
    }

    SendMessage(m_pencere, WM_SETICON, ICON_BIG, (LPARAM)buyukIkon); // Görev çubuğu ve Alt+Tab'da gösterilen büyük simge, pencereye açıkça bildirilir
    SendMessage(m_pencere, WM_SETICON, ICON_SMALL, (LPARAM)kucukIkon); // Başlık çubuğu/sistem menüsünde gösterilen küçük simge, pencereye açıkça bildirilir

    m_baslangicZamani = GetTickCount64(); // Programın açıldığı an kaydedilir, Hakkında bölümünde çalışma süresini göstermek için kullanılacaktır

    // Pencere ilk oluşturulduğunda, Windows başlık çubuğunun çerçevesini WM_NCCALCSIZE bastırmamız devreye girmeden hesaplayıp bir kerelik beyaz bir şerit çizebilir, bu satır Windows'a çerçeveyi bizim bastırma mantığımızla yeniden hesaplamasını zorlar
    SetWindowPos(m_pencere, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    ShowWindow(m_pencere, SW_SHOW); // Pencere ekranda görünür hale getirilir
    SetTimer(m_pencere, SekmeUykuZamanlayiciId, 30000, nullptr); // Otuz saniyede bir, hareketsiz sekmelerin uykuya alınıp alınmayacağı kontrol edilecek şekilde bir zamanlayıcı kurulur
    BildirimSimgesiOlustur(); // İndirme tamamlanma bildirimleri için kullanılacak, görünmez bildirim alanı simgesi oluşturulur
    WebView2OrtamiBaslat(); // WebView2 ortamının asenkron olarak hazırlanması başlatılır
    return true; // Pencere başarıyla oluşturuldu
}

// Pencere tam ekran değilken, sol ve sağ kenarda Windows'un kendi (görünmez) boyutlandırma çerçevesi için ayrılan piksel kalınlığı
// WM_NCCALCSIZE içinde istemci alanı bu kadar içeri çekilerek, bu şerit WebView2'ye değil doğrudan Windows'un standart kenar boyutlandırma mantığına bırakılır
static int KenarTutmaPayi()
{
    return GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
}

// Windows, bu sınıftan oluşan bir pencereye her mesaj geldiğinde önce bu statik fonksiyonu çağırır
LRESULT CALLBACK BrowserWindow::StatikPencereMesajIsleyici(HWND pencere, UINT mesaj, WPARAM birinciParametre, LPARAM ikinciParametre)
{
    BrowserWindow* kendisi = nullptr; // Bu pencereye ait BrowserWindow nesnesinin işaretçisini tutacak değişken

    if (mesaj == WM_NCCREATE) // Pencere ilk oluşturulduğu anda gelen özel bir mesaj ise
    {
        CREATESTRUCT* olusturmaBilgisi = reinterpret_cast<CREATESTRUCT*>(ikinciParametre); // Pencere oluşturulurken CreateWindowEx'e verilen bilgilere erişilir
        kendisi = reinterpret_cast<BrowserWindow*>(olusturmaBilgisi->lpCreateParams); // Create fonksiyonunda gönderdiğimiz this işaretçisi buradan geri alınır
        SetWindowLongPtr(pencere, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(kendisi)); // Bu işaretçi, pencereye ait özel veri alanına kalıcı olarak kaydedilir
    }
    else // Pencere zaten oluşturulduktan sonra gelen diğer tüm mesajlar için
    {
        kendisi = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(pencere, GWLP_USERDATA)); // Daha önce kaydedilmiş işaretçi, pencerenin özel veri alanından geri okunur
    }

    if (kendisi != nullptr) // Eğer bu pencereye ait bir BrowserWindow nesnesi bulunabildiyse
    {
        return kendisi->PencereMesajIsle(pencere, mesaj, birinciParametre, ikinciParametre); // Mesajın asıl işlenmesi, nesnenin kendi metoduna bırakılır
    }

    return DefWindowProc(pencere, mesaj, birinciParametre, ikinciParametre); // Nesne henüz bulunamadıysa, mesaj Windows'un varsayılan işleyicisine bırakılır
}

// Bu fonksiyon, bu pencereye gelen her mesajı asıl olarak işleyen fonksiyondur
LRESULT BrowserWindow::PencereMesajIsle(HWND pencere, UINT mesaj, WPARAM birinciParametre, LPARAM ikinciParametre)
{
    if (mesaj == WM_NCCALCSIZE && birinciParametre == TRUE) // Pencerenin çerçeve dışı (başlık çubuğu, kenarlık) alanı hesaplanırken gelen mesaj ise
    {
        // Üstte hiç boşluk bırakılmaz, yani Windows'un kendi beyaz başlık çubuğu hiç çizilmez (WS_CAPTION sadece küçült/büyüt animasyonları için stilde tutuluyor)
        // Ama pencere büyütülmüş değilse sol/sağ/alttan, Windows'un GERÇEK boyutlandırma çerçevesi kadar (SM_CXSIZEFRAME + SM_CXPADDEDBORDER) içeri çekilir
        // Bu şerit hiçbir WebView2 tarafından kaplanmaz, biz de hiçbir şey boyamayız: orayı doğrudan Windows'un kendi görünmez boyutlandırma çerçevesi olarak yönetmesine bırakırız, WM_NCHITTEST de bunun için otomatik çalışır, elle kod yazmaya gerek kalmaz
        if (!IsZoomed(pencere))
        {
            NCCALCSIZE_PARAMS* hesapBilgisi = reinterpret_cast<NCCALCSIZE_PARAMS*>(ikinciParametre);
            int payX = KenarTutmaPayi();
            int payY = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
            hesapBilgisi->rgrc[0].left += payX;
            hesapBilgisi->rgrc[0].right -= payX;
            hesapBilgisi->rgrc[0].bottom -= payY;
        }
        return 0; // Üstteki çerçeve çizimi tamamen iptal edilir, sol/sağ/alttaki ise yukarıda ayrıldıysa Windows'a bırakılır
    }

    if (mesaj == WM_SIZE) // Pencere boyutu değiştiğinde gelen mesaj ise
    {
        BoyutlariGuncelle(); // Toolbar ve aktif sekmenin WebView2'lerinin yeni pencere boyutuna göre yeniden konumlanması sağlanır
        ToolbaraMesajGonder(L"{\"type\":\"windowstate\",\"maximized\":" + std::wstring(birinciParametre == SIZE_MAXIMIZED ? L"true" : L"false") + L"}"); // Pencerenin büyütülmüş olup olmadığı toolbar arayüzüne bildirilir, böylece büyüt/geri yükle simgesi doğru gösterilir
        return 0; // Mesaj işlendi
    }

    if (mesaj == WM_SETFOCUS) // Pencere, kullanıcı tarafından tekrar öne getirilip odaklandığında gelen mesaj ise (örneğin başka bir uygulamadan Alt+Tab ile geri dönüldüğünde)
    {
        // Odak, doğrudan aktif sekmenin WebView2'sine verilir, böylece kullanıcı pencereye tıklamadan hemen klavye kısayollarını kullanabilir, önce içeriğe tıklama zorunluluğu ortadan kalkar
        SekmeBilgisi* aktifSekme = AktifSekmeyiAl(); // Odağın verileceği sekme, aktif sekme olarak alınır
        if (aktifSekme != nullptr && aktifSekme->denetleyici != nullptr) // Eğer bir aktif sekme varsa ve denetleyicisi hazırsa
        {
            aktifSekme->denetleyici->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC); // Odak, bu sekmenin içeriğine taşınır
        }
        else if (m_toolbarDenetleyici != nullptr) // Eğer hiç sekme yoksa ama toolbar hazırsa
        {
            m_toolbarDenetleyici->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC); // Odak, toolbar'a taşınır
        }
        return 0; // Mesaj işlendi
    }

    if (mesaj == WM_TIMER && birinciParametre == SekmeUykuZamanlayiciId) // Periyodik uyku kontrolü zamanlayıcısı tetiklendiyse
    {
        UykudaSekmeleriKontrolEt(); // Uzun süredir kullanılmayan sekmeler kontrol edilip gerekiyorsa uykuya alınır
        YedekSekmeyiGerekirseKapat(); // Uzun süredir kullanılmayan yedek sekme denetleyicisi de aynı periyodik kontrolde değerlendirilir
        return 0; // Mesaj işlendi
    }

    if (mesaj == WM_CLOSE && !m_cikisTemizligiYapildi) // Pencere kapatılmak isteniyorsa ve çıkış temizliği daha önce yapılmadıysa (WebView2 hâlâ canlıyken, WM_DESTROY'dan önce bu noktada yapılmalı)
    {
        m_cikisTemizligiYapildi = true; // Bu bayrak, DestroyWindow aşağıda tekrar WM_CLOSE tetiklemese de olası çift çalışmayı engeller
        AyarVerisi ayarlar = Ayarlar::Oku();
        if (ayarlar.cikistaTemizle) // Kullanıcı, kapanışta otomatik temizleme istemişse
        {
            std::vector<std::wstring> haricTutulanlar; // "|" ile ayrılmış hariç tutulan site listesi, ayrı parçalara bölünür
            size_t baslangic = 0;
            while (baslangic < ayarlar.haricTutulanSiteler.size())
            {
                size_t ayrac = ayarlar.haricTutulanSiteler.find(L'|', baslangic);
                if (ayrac == std::wstring::npos) { ayrac = ayarlar.haricTutulanSiteler.size(); }
                if (ayrac > baslangic) { haricTutulanlar.push_back(ayarlar.haricTutulanSiteler.substr(baslangic, ayrac - baslangic)); }
                baslangic = ayrac + 1;
            }

            if (ayarlar.cikistaTemizleKategoriler.find(L"gecmis") != std::wstring::npos) // "Gezinme geçmişi" kategorisi seçiliyse, kendi history.json dosyamız (hariç tutulanlar korunarak) temizlenir
            {
                Depolama::GecmisiTemizleHaricTutarak(L"", haricTutulanlar);
            }

            // WebView2'nin kendi verileri (çerez/önbellek) site bazlı hariç tutmayı desteklemediği için, bu kategoriler seçiliyse tüm siteler için temizlenir
            COREWEBVIEW2_BROWSING_DATA_KINDS bayraklar = static_cast<COREWEBVIEW2_BROWSING_DATA_KINDS>(0);
            if (ayarlar.cikistaTemizleKategoriler.find(L"cerez") != std::wstring::npos) { bayraklar = bayraklar | COREWEBVIEW2_BROWSING_DATA_KINDS_COOKIES; }
            if (ayarlar.cikistaTemizleKategoriler.find(L"onbellek") != std::wstring::npos) { bayraklar = bayraklar | COREWEBVIEW2_BROWSING_DATA_KINDS_CACHE_STORAGE | COREWEBVIEW2_BROWSING_DATA_KINDS_DISK_CACHE; }
            if (bayraklar != 0 && m_toolbarWebView != nullptr)
            {
                ComPtr<ICoreWebView2_13> webViewSurum13;
                m_toolbarWebView.As(&webViewSurum13);
                if (webViewSurum13 != nullptr)
                {
                    ComPtr<ICoreWebView2Profile> profil;
                    webViewSurum13->get_Profile(&profil);
                    ComPtr<ICoreWebView2Profile2> profilSurum2;
                    if (profil != nullptr) { profil.As(&profilSurum2); }
                    if (profilSurum2 != nullptr)
                    {
                        // Pencere zaten kapanıyor olduğu için tamamlanma bildirimini beklemeye gerek yoktur, işlem arka planda devam ederken program kapanış sürecine devam eder
                        profilSurum2->ClearBrowsingDataInTimeRange(bayraklar, 0.0, static_cast<double>(time(nullptr)), Callback<ICoreWebView2ClearBrowsingDataCompletedHandler>([](HRESULT) -> HRESULT { return S_OK; }).Get());
                    }
                }
            }
        }
    }

    if (mesaj == WM_DESTROY) // Pencere kapatıldığında gelen mesaj ise
    {
        *m_pencereGecerliMi = false; // Beklemede olan asenkron WebView2 geri çağırmalarına (örneğin sekme önizlemesi yakalama), artık bu pencereye güvenle erişilemeyeceği bildirilir
        KillTimer(m_pencere, SekmeUykuZamanlayiciId); // Artık gerekmediği için zamanlayıcı durdurulur

        NOTIFYICONDATA bildirimVerisi = {}; // Bildirim alanı simgesi, program kapanırken temizlenir, aksi halde simge görev çubuğunda askıda kalabilir
        bildirimVerisi.cbSize = sizeof(NOTIFYICONDATA);
        bildirimVerisi.hWnd = m_pencere;
        bildirimVerisi.uID = 1;
        Shell_NotifyIcon(NIM_DELETE, &bildirimVerisi);

        PostQuitMessage(0); // Programın mesaj döngüsüne, tamamen sonlanması gerektiği bildirilir
        return 0; // Mesaj işlendi
    }

    if (mesaj == DevToolsHedefBulunduMesaji) // DevTools paneli için, arka planda çalışan hedef bulma iş parçacığı sonucunu bildirdiyse
    {
        std::unique_ptr<std::wstring> devToolsOnYuzAdresi(reinterpret_cast<std::wstring*>(ikinciParametre)); // Mesajın taşıdığı işaretçinin sahipliği burada devralınır, fonksiyondan çıkılırken (hangi yoldan çıkılırsa çıkılsın) otomatik olarak "delete" edilir
        if (m_devToolsAcikMi && m_devToolsWebView != nullptr && !devToolsOnYuzAdresi->empty()) // Panel hâlâ açık tutulmak isteniyorsa (kullanıcı beklerken F12'ye tekrar basıp vazgeçmediyse) ve bir hedef bulunabildiyse
        {
            // Bu adres, tamamen yerel hata ayıklama sunucusundan (127.0.0.1) sunulan gerçek DevTools arayüzüne işaret eder, hiçbir internet bağlantısı gerekmez
            m_devToolsWebView->Navigate(devToolsOnYuzAdresi->c_str());
        }
        return 0; // Mesaj işlendi
    }

    if (mesaj == SizintiSonucuMesaji) // Şifre sızıntı/zayıflık kontrolü için, arka planda çalışan iş parçacığı sonucunu bildirdiyse
    {
        std::unique_ptr<SizintiSonucu> sonuc(reinterpret_cast<SizintiSonucu*>(ikinciParametre)); // Mesajın taşıdığı işaretçinin sahipliği burada devralınır
        std::wstring cevapMesaji = L"{\"type\":\"breachresult\",\"site\":\"" + JsonIcinKacisla(sonuc->site) + L"\",\"username\":\"" + JsonIcinKacisla(sonuc->kullaniciAdi) + L"\",\"authFailed\":" + (sonuc->dogrulamaBasarisizMi ? L"true" : L"false") + L",\"weak\":" + (sonuc->zayifMi ? L"true" : L"false") + L",\"breached\":" + (sonuc->sizdirilmisMi ? L"true" : L"false") + L",\"count\":" + std::to_wstring(sonuc->sizintiSayisi) + L"}";
        SekmeyeMesajGonder(sonuc->sekmeId, cevapMesaji); // Sonuç, isteği yapan Ayarlar sekmesine gönderilir
        return 0; // Mesaj işlendi
    }

    if (mesaj == MagazaEklentiSonucuMesaji) // Mağaza adresinden uzantı indirme/paket açma işlemi için, arka planda çalışan iş parçacığı sonucunu bildirdiyse
    {
        std::unique_ptr<MagazaEklentiSonucu> sonuc(reinterpret_cast<MagazaEklentiSonucu*>(ikinciParametre)); // Mesajın taşıdığı işaretçinin sahipliği burada devralınır
        if (!sonuc->basarili)
        {
            SekmeyeMesajGonder(sonuc->sekmeId, L"{\"type\":\"store_extension_result\",\"success\":false,\"error\":\"" + JsonIcinKacisla(sonuc->hataMesaji) + L"\"}");
            return 0;
        }

        // Paket açma işlemi arka planda tamamlandı, şimdi AddBrowserExtension ANA iş parçacığında (WebView2/COM nesnelerine sadece buradan erişilebilir) çağrılır
        ComPtr<ICoreWebView2Profile7> profil = EklentiProfiliniAl(m_toolbarWebView.Get());
        if (profil == nullptr)
        {
            SekmeyeMesajGonder(sonuc->sekmeId, L"{\"type\":\"store_extension_result\",\"success\":false,\"error\":\"Eklenti profiline erişilemedi.\"}");
            return 0;
        }

        int sekmeId = sonuc->sekmeId;
        profil->AddBrowserExtension(
            sonuc->klasorYolu.c_str(),
            Callback<ICoreWebView2ProfileAddBrowserExtensionCompletedHandler>(
                [this, sekmeId, gecerlilikBayragi = m_pencereGecerliMi](HRESULT sonucKodu, ICoreWebView2BrowserExtension* eklenti) -> HRESULT
                {
                    if (!*gecerlilikBayragi) { return S_OK; }
                    bool basarili = SUCCEEDED(sonucKodu) && eklenti != nullptr;
                    SekmeyeMesajGonder(sekmeId, L"{\"type\":\"store_extension_result\",\"success\":" + std::wstring(basarili ? L"true" : L"false") + (basarili ? L"" : L",\"error\":\"WebView2 bu paketi uzantı olarak kabul etmedi.\"") + L"}");
                    if (basarili) { EklentileriniGonder(sekmeId); }
                    return S_OK;
                }
            ).Get()
        );
        return 0; // Mesaj işlendi
    }

    return DefWindowProc(pencere, mesaj, birinciParametre, ikinciParametre); // Diğer tüm mesajlar, Windows'un varsayılan işleyicisine bırakılır
}

// Bu fonksiyon, tüm WebView2 örneklerinin paylaşacağı ortak ortamı oluşturmayı başlatır
void BrowserWindow::WebView2OrtamiBaslat()
{
    auto ortamSecenekleri = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>(); // Ortam oluşturulurken ekstra tarayıcı başlangıç parametreleri vermek için kullanılan seçenekler nesnesi
    // Chrome DevTools Protokolü'nün bu yerel bağlantı noktasında sunulması sağlanır, gömülü DevTools paneli (m_devToolsWebView) bu porttan gelen gerçek DevTools arayüzünü gösterir
    // Bu port sadece 127.0.0.1 (bu bilgisayarın kendisi) üzerinden erişilebilir, dışarıdan ulaşılamaz
    ortamSecenekleri->put_AdditionalBrowserArguments((L"--remote-debugging-port=" + std::to_wstring(DevToolsUzaktanHataAyiklamaPortu)).c_str());
    ortamSecenekleri->put_Language(Ayarlar::Oku().tarayiciDili.c_str()); // Site içeriği dili tercihini (Accept-Language) ve Chromium'un otomatik seçtiği yazım denetimi sözlüğünü belirler, sadece ortam ilk oluşturulduğunda okunur, bu yüzden değişikliğin etkili olması için program yeniden başlatılmalıdır

    ComPtr<ICoreWebView2EnvironmentOptions6> ortamSecenekleriSurum6; // Paketlenmemiş tarayıcı uzantılarına izin verme seçeneği, bu arayüz sürümünde bulunur
    ortamSecenekleri.As(&ortamSecenekleriSurum6);
    if (ortamSecenekleriSurum6 != nullptr)
    {
        ortamSecenekleriSurum6->put_AreBrowserExtensionsEnabled(TRUE); // Kullanıcının Ayarlar sayfasından klasörden yükleyebileceği paketlenmemiş uzantılara izin verilir
    }

    // ÖNEMLİ: Hiçbir profil seçili değilse (normal, varsayılan kullanım), WebView2'nin KENDİ varsayılan kullanıcı verisi klasörü konumu (nullptr) olduğu gibi korunur
    // Aksi halde mevcut kullanıcıların çerezleri/oturumları/geçmişi, program güncellendiğinde farklı bir klasöre taşınmış gibi görünüp "kaybolurdu"; özel klasör yolu SADECE bir profil açıkça seçildiğinde (--profil= ile başlatıldığında) kullanılır
    std::wstring aktifProfil = AktifProfilAdiReferansi();
    std::wstring kullaniciVerisiKlasoru = aktifProfil.empty() ? L"" : (VeriKlasoruAl() + L"\\webview2data");
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, aktifProfil.empty() ? nullptr : kullaniciVerisiKlasoru.c_str(), ortamSecenekleri.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT sonuc, ICoreWebView2Environment* ortam) -> HRESULT
            {
                m_webViewOrtami = ortam; // Oluşan ortam, bu sınıfın ortam değişkenine kaydedilir
                ToolbarWebViewOlustur(); // Ortam hazır olduğuna göre, toolbar arayüzünü gösterecek WebView2 oluşturulabilir
                MenuWebViewOlustur(); // Hamburger menü panelini gösterecek ayrı, küçük WebView2 de aynı anda oluşturulmaya başlanır
                DevToolsWebViewOlustur(); // Gömülü DevTools panelini gösterecek ayrı WebView2 de aynı anda oluşturulmaya başlanır
                IzinWebViewOlustur(); // Site izin bildirim panelini gösterecek ayrı, küçük WebView2 de aynı anda oluşturulmaya başlanır
                SiteBilgisiWebViewOlustur(); // Kilit ikonuna tıklanınca açılan site bilgisi/izinleri paneli de aynı anda oluşturulmaya başlanır
                SertifikaWebViewOlustur(); // Sertifika hatası bildirim paneli de aynı anda oluşturulmaya başlanır
                DoldurmaWebViewOlustur(); // Otomatik doldurma paneli de aynı anda oluşturulmaya başlanır
                OneriWebViewOlustur(); // Adres çubuğu öneri paneli de aynı anda oluşturulmaya başlanır

                // Kullanıcı daha önce karşılama ekranını hiç görmediyse, programın ilk sekmesi hızlı erişim sayfası yerine karşılama ekranı olur, bu ekran tamamlanınca bir daha asla gösterilmez
                EngellemeAyarlariniYenile(); // Reklam/izleyici engelleme ayarları, ilk sekme açılmadan önce önbelleğe alınır, böylece ilk sekmenin istekleri de doğru şekilde filtrelenir
                AyarVerisi baslangicAyarlari = Ayarlar::Oku();
                m_rizaOnayiBekleniyor = (baslangicAyarlari.onaylananRizaSurumu != ProgramSurumu::Al()); // Kullanıcı, çalışan .exe'nin güncel sürümü için henüz rıza onayı vermediyse (ilk kurulumda alan boş olduğu için otomatik eşitsizdir), karşılama ekranı gösterilecek ve program kullanılamayacaktır
                bool karsilamaGosterilsinMi = m_rizaOnayiBekleniyor; // Daha önce tamamlanmadıysa, karşılama ekranı gösterilecektir
                std::vector<SayfaKaydi> onceki_oturum = (!karsilamaGosterilsinMi && baslangicAyarlari.sonOturumuGeriYukle) ? Depolama::OturumuOku() : std::vector<SayfaKaydi>(); // Karşılama ekranı gösterilmeyecekse ve kullanıcı bu özelliği açtıysa, önceki oturumun sekmeleri okunur

                if (!onceki_oturum.empty()) // Geri yüklenecek, en az bir sekme içeren bir önceki oturum varsa
                {
                    for (size_t i = 0; i < onceki_oturum.size(); i++) // Kaydedilmiş her adres, kendi sekmesinde, ilk sekme aktif olacak şekilde sırayla açılır
                    {
                        YeniSekmeOlustur(onceki_oturum[i].adres, i == 0, false);
                    }
                }
                else // Geri yüklenecek bir oturum yoksa (veya özellik kapalıysa), her zamanki gibi tek bir başlangıç sekmesi açılır
                {
                    std::wstring baslangicAdresi = karsilamaGosterilsinMi ? L"https://tarayici.local/onboarding.html" : L"https://tarayici.local/newtab.html"; // Duruma göre doğru dahili sayfa seçilir
                    int acilanSekmeId = YeniSekmeOlustur(baslangicAdresi, true, true); // Program ilk açıldığında, bu dahili sayfayla bir ilk sekme açılır ve aktif yapılır
                    if (karsilamaGosterilsinMi) { m_onboardingSekmeId = acilanSekmeId; } // Rıza onayı bekleniyorsa, bu sekmenin kimliği hatırlanır: kapatılamayacak ve diğer tüm sekme/navigasyon denemeleri buraya yönlendirilecektir
                }
                YedekSekmeHazirla(); // Kullanıcı daha hiç "+" butonuna basmadan, bir sonraki sekme için arka planda şimdiden bir yedek denetleyici hazırlanmaya başlanır
                return S_OK; // İşlemin başarıyla ele alındığı bildirilir
            }
        ).Get()
    );
}

// Bu fonksiyon, toolbar arayüzünü (sekme şeridi, geri/ileri/yenile/ana sayfa/ayarlar butonları ve adres çubuğu) gösterecek WebView2'yi oluşturur
void BrowserWindow::ToolbarWebViewOlustur()
{
    m_webViewOrtami->CreateCoreWebView2Controller(
        m_pencere,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this](HRESULT sonuc, ICoreWebView2Controller* denetleyici) -> HRESULT
            {
                m_toolbarDenetleyici = denetleyici; // Oluşan denetleyici, bu sınıfın toolbar denetleyici değişkenine kaydedilir
                m_toolbarDenetleyici->get_CoreWebView2(&m_toolbarWebView); // Denetleyicinin içindeki asıl WebView2 nesnesi alınır

                std::wstring varliklarKlasoru = CalismaKlasoruBul() + L"\\assets"; // Toolbar ve Ayarlar sayfalarının HTML dosyalarının bulunduğu klasörün tam yolu hesaplanır

                ComPtr<ICoreWebView2_3> toolbarWebViewSurum3; // SetVirtualHostNameToFolderMapping fonksiyonu daha yeni bir arayüz sürümünde bulunduğu için
                m_toolbarWebView.As(&toolbarWebViewSurum3); // Elimizdeki WebView2 nesnesinin bu daha yeni sürümüne geçiş yapılır
                toolbarWebViewSurum3->SetVirtualHostNameToFolderMapping(L"tarayici.local", varliklarKlasoru.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW); // Sahte güvenli adres ile gerçek klasör eşleştirilir

                // Program her açılışta, kendi dahili sayfalarımızın (chrome.html, newtab.html, settings.html gibi) disk önbelleğini bir kere temizliyoruz
                // Aksi halde, tarayici.local üzerinden servis edilen bu dosyalar Chromium'un normal HTTP önbelleğine girer, biz dosyayı diskte güncellesek bile (örneğin yeni bir derleme sonrası) eski önbellekteki sürüm günlerce gösterilmeye devam edebilir
                // Bu temizlik tüm profil için tek seferliktir (profilin paylaşılan tek önbelleği olduğu için bir WebView2 üzerinden tetiklenmesi yeterlidir), gerçek internet sitelerinin önbelleğini de temizler ama bu, programın hâlâ geliştirme aşamasında olması göz önüne alınınca kabul edilebilir bir bedeldir
                ComPtr<ICoreWebView2_13> toolbarWebViewSurum13;
                m_toolbarWebView.As(&toolbarWebViewSurum13);
                if (toolbarWebViewSurum13)
                {
                    ComPtr<ICoreWebView2Profile> profil;
                    toolbarWebViewSurum13->get_Profile(&profil);
                    ComPtr<ICoreWebView2Profile2> profilSurum2;
                    if (profil) { profil.As(&profilSurum2); }
                    if (profilSurum2)
                    {
                        profilSurum2->ClearBrowsingDataInTimeRange(COREWEBVIEW2_BROWSING_DATA_KINDS_DISK_CACHE, 0.0, static_cast<double>(time(nullptr)),
                            Callback<ICoreWebView2ClearBrowsingDataCompletedHandler>([](HRESULT) -> HRESULT { return S_OK; }).Get());
                    }
                }

                m_toolbarWebView->add_WebMessageReceived(
                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                        [this](ICoreWebView2* gonderen, ICoreWebView2WebMessageReceivedEventArgs* argumanlar) -> HRESULT
                        {
                            LPWSTR mesajMetni = nullptr; // Gelen mesajın düz metin halini tutacak değişken
                            if (SUCCEEDED(argumanlar->TryGetWebMessageAsString(&mesajMetni))) // Mesaj düz bir metin olarak başarıyla okunabildiyse
                            {
                                ToolbarMesajiIsle(mesajMetni); // Mesajın içeriğine göre doğru işlem yapılması için ilgili fonksiyona gönderilir
                                CoTaskMemFree(mesajMetni); // WebView2'nin bizim için ayırdığı bellek serbest bırakılır
                            }
                            return S_OK; // Mesaj başarıyla ele alındı
                        }
                    ).Get(),
                    nullptr
                );

                // Toolbar odaktayken de (örneğin adres çubuğu seçiliyken) klavye kısayollarının çalışması için, bu denetleyiciye de kısayol dinleyicisi bağlanır
                m_toolbarDenetleyici->add_AcceleratorKeyPressed(
                    Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
                        [this](ICoreWebView2Controller* gonderen, ICoreWebView2AcceleratorKeyPressedEventArgs* argumanlar) -> HRESULT
                        {
                            COREWEBVIEW2_KEY_EVENT_KIND olayTuru; // Bu tuş olayının türünü (basma, bırakma gibi) tutacak değişken
                            argumanlar->get_KeyEventKind(&olayTuru); // Olay türü okunur
                            if (olayTuru != COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN) // Sadece tuşa ilk basıldığı an ile ilgileniyoruz, tekrar sinyalleri ve bırakma olayları yok sayılır
                            {
                                return S_OK; // Başka bir şey yapmaya gerek yok
                            }
                            UINT32 tusKodu = 0; // Basılan tuşun kodunu tutacak değişken
                            argumanlar->get_VirtualKey(&tusKodu); // Tuş kodu okunur
                            if (KisayolTusuIsle(tusKodu)) // Eğer bu tuş, tarayıcıya özel bir kısayolsa ve işlendiyse
                            {
                                argumanlar->put_Handled(TRUE); // Tuşun adres çubuğuna veya sayfaya gitmesi engellenir
                            }
                            return S_OK; // İşlem başarıyla ele alındı
                        }
                    ).Get(),
                    nullptr
                );

                m_toolbarWebView->Navigate(L"https://tarayici.local/chrome.html"); // Toolbar arayüzünün HTML dosyası, sahte yerel adres üzerinden açılır
                BoyutlariGuncelle(); // Toolbar WebView2'si oluştuğuna göre, mevcut pencere boyutuna göre doğru konuma yerleştirilir
                return S_OK; // İşlem başarıyla ele alındı
            }
        ).Get()
    );
}

// Bu fonksiyon, hamburger menü panelini gösterecek AYRI, küçük WebView2'yi oluşturur
// Bu denetleyici, toolbar'dan tamamen bağımsızdır: kendi dikdörtgeni sadece panelin gerçek boyutu kadardır, bu yüzden panel içeriğin üzerine bindiğinde ne içerik kayar ne de altındaki sayfa örtülür (panelin dışı zaten normal şartlarda da başka bir şey göstermiyordu)
void BrowserWindow::MenuWebViewOlustur()
{
    m_webViewOrtami->CreateCoreWebView2Controller(
        m_pencere,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this](HRESULT sonuc, ICoreWebView2Controller* denetleyici) -> HRESULT
            {
                m_menuDenetleyici = denetleyici; // Oluşan denetleyici, bu sınıfın menü denetleyici değişkenine kaydedilir
                m_menuDenetleyici->get_CoreWebView2(&m_menuWebView); // Denetleyicinin içindeki asıl WebView2 nesnesi alınır
                m_menuDenetleyici->put_IsVisible(FALSE); // Panel, menü açılana kadar görünmez tutulur
                // NOT: put_DefaultBackgroundColor ile saydamlık denendi ama WebView2 bu pencerede (DirectComposition kurulu olmadığı için) saydam alanı SİYAH boyuyor;
                // bu yüzden köşe yuvarlama, aşağıdaki MenuPaneliKoseleriniYuvarlaYap() fonksiyonunda gerçek pencere bölgesini (SetWindowRgn) kırparak yapılıyor, renk hilesiyle değil

                std::wstring varliklarKlasoru = CalismaKlasoruBul() + L"\\assets"; // Menü panelinin HTML dosyasının bulunduğu klasörün tam yolu

                ComPtr<ICoreWebView2_3> menuWebViewSurum3; // SetVirtualHostNameToFolderMapping fonksiyonu daha yeni bir arayüz sürümünde bulunduğu için
                m_menuWebView.As(&menuWebViewSurum3);
                menuWebViewSurum3->SetVirtualHostNameToFolderMapping(L"tarayici.local", varliklarKlasoru.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW); // Sahte güvenli adres ile gerçek klasör eşleştirilir

                // Menüdeki öğelere tıklamalar ("newtab", "opensettings|gecmis" gibi), toolbar'dakiyle AYNI komut işleyicisine gönderilir, komutların anlamı nereden geldiğine göre değişmez
                m_menuWebView->add_WebMessageReceived(
                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                        [this](ICoreWebView2* gonderen, ICoreWebView2WebMessageReceivedEventArgs* argumanlar) -> HRESULT
                        {
                            LPWSTR mesajMetni = nullptr;
                            if (SUCCEEDED(argumanlar->TryGetWebMessageAsString(&mesajMetni)))
                            {
                                ToolbarMesajiIsle(mesajMetni);
                                CoTaskMemFree(mesajMetni);
                            }
                            return S_OK;
                        }
                    ).Get(),
                    nullptr
                );

                // Kullanıcı menü açıkken başka bir yere (içeriğe, toolbar'a, pencere dışına) tıklayıp odağı kaybettirince, gerçek bir açılır menü gibi otomatik olarak kapanır
                m_menuDenetleyici->add_LostFocus(
                    Callback<ICoreWebView2FocusChangedEventHandler>(
                        [this](ICoreWebView2Controller* gonderen, IUnknown* argumanlar) -> HRESULT
                        {
                            MenuyuKapat(); // Odak kaybedilince menü kapatılır
                            return S_OK;
                        }
                    ).Get(),
                    nullptr
                );

                m_menuWebView->Navigate(L"https://tarayici.local/menu.html"); // Menü panelinin HTML dosyası, sahte yerel adres üzerinden açılır
                return S_OK;
            }
        ).Get()
    );
}

// Bu fonksiyon, gömülü DevTools panelini gösterecek AYRI denetleyiciyi oluşturur
// Menü panelinden farklı olarak bu denetleyici, hiçbir sahte yerel adrese (tarayici.local) gezindirilmez, çünkü gösterdiği içerik bizim kendi HTML dosyalarımız değil, WebView2'nin kendi uzaktan hata ayıklama sunucusunun servis ettiği gerçek Chrome DevTools arayüzüdür
void BrowserWindow::DevToolsWebViewOlustur()
{
    m_webViewOrtami->CreateCoreWebView2Controller(
        m_pencere,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this, gecerlilikBayragi = m_pencereGecerliMi](HRESULT sonuc, ICoreWebView2Controller* denetleyici) -> HRESULT
            {
                if (!*gecerlilikBayragi) // Pencere, bu denetleyici hazır olmadan önce kapatılmaya başladıysa, "this" işaretçisine hiç dokunulmadan güvenle vazgeçilir
                {
                    return S_OK;
                }
                m_devToolsDenetleyici = denetleyici; // Oluşan denetleyici, bu sınıfın DevTools denetleyici değişkenine kaydedilir
                m_devToolsDenetleyici->get_CoreWebView2(&m_devToolsWebView); // Denetleyicinin içindeki asıl WebView2 nesnesi alınır
                m_devToolsDenetleyici->put_IsVisible(FALSE); // Panel, F12'ye basılana kadar görünmez tutulur
                return S_OK;
            }
        ).Get()
    );
}

// Bu fonksiyon, site izin bildirim panelini gösterecek AYRI denetleyiciyi oluşturur
void BrowserWindow::IzinWebViewOlustur()
{
    m_webViewOrtami->CreateCoreWebView2Controller(
        m_pencere,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this, gecerlilikBayragi = m_pencereGecerliMi](HRESULT sonuc, ICoreWebView2Controller* denetleyici) -> HRESULT
            {
                if (!*gecerlilikBayragi) { return S_OK; } // Pencere, bu denetleyici hazır olmadan önce kapatılmaya başladıysa, "this" işaretçisine hiç dokunulmadan güvenle vazgeçilir

                m_izinDenetleyici = denetleyici; // Oluşan denetleyici, bu sınıfın izin denetleyici değişkenine kaydedilir
                m_izinDenetleyici->get_CoreWebView2(&m_izinWebView); // Denetleyicinin içindeki asıl WebView2 nesnesi alınır
                m_izinDenetleyici->put_IsVisible(FALSE); // Panel, bir izin isteği gelene kadar görünmez tutulur

                std::wstring varliklarKlasoru = CalismaKlasoruBul() + L"\\assets"; // İzin panelinin HTML dosyasının bulunduğu klasörün tam yolu

                ComPtr<ICoreWebView2_3> izinWebViewSurum3; // SetVirtualHostNameToFolderMapping fonksiyonu daha yeni bir arayüz sürümünde bulunduğu için
                m_izinWebView.As(&izinWebViewSurum3);
                izinWebViewSurum3->SetVirtualHostNameToFolderMapping(L"tarayici.local", varliklarKlasoru.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW); // Sahte güvenli adres ile gerçek klasör eşleştirilir

                // Paneldeki "İzin Ver"/"Engelle" tıklamaları, toolbar'dakiyle AYNI komut işleyicisine gönderilir, komutların anlamı nereden geldiğine göre değişmez
                m_izinWebView->add_WebMessageReceived(
                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                        [this](ICoreWebView2* gonderen, ICoreWebView2WebMessageReceivedEventArgs* argumanlar) -> HRESULT
                        {
                            LPWSTR mesajMetni = nullptr;
                            if (SUCCEEDED(argumanlar->TryGetWebMessageAsString(&mesajMetni)))
                            {
                                ToolbarMesajiIsle(mesajMetni);
                                CoTaskMemFree(mesajMetni);
                            }
                            return S_OK;
                        }
                    ).Get(),
                    nullptr
                );

                m_izinWebView->Navigate(L"https://tarayici.local/izin-bildirimi.html"); // İzin panelinin HTML dosyası, sahte yerel adres üzerinden açılır
                return S_OK;
            }
        ).Get()
    );
}

// Bu fonksiyon, adres çubuğundaki kilit ikonuna tıklanınca açılan site bilgisi/izinleri panelini gösterecek AYRI denetleyiciyi oluşturur
void BrowserWindow::SiteBilgisiWebViewOlustur()
{
    m_webViewOrtami->CreateCoreWebView2Controller(
        m_pencere,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this, gecerlilikBayragi = m_pencereGecerliMi](HRESULT sonuc, ICoreWebView2Controller* denetleyici) -> HRESULT
            {
                if (!*gecerlilikBayragi) { return S_OK; } // Pencere, bu denetleyici hazır olmadan önce kapatılmaya başladıysa, "this" işaretçisine hiç dokunulmadan güvenle vazgeçilir

                m_sitePaneliDenetleyici = denetleyici; // Oluşan denetleyici, bu sınıfın site bilgisi denetleyici değişkenine kaydedilir
                m_sitePaneliDenetleyici->get_CoreWebView2(&m_sitePaneliWebView); // Denetleyicinin içindeki asıl WebView2 nesnesi alınır
                m_sitePaneliDenetleyici->put_IsVisible(FALSE); // Panel, kilit ikonuna tıklanana kadar görünmez tutulur

                std::wstring varliklarKlasoru = CalismaKlasoruBul() + L"\\assets"; // Panelin HTML dosyasının bulunduğu klasörün tam yolu

                ComPtr<ICoreWebView2_3> sitePaneliWebViewSurum3; // SetVirtualHostNameToFolderMapping fonksiyonu daha yeni bir arayüz sürümünde bulunduğu için
                m_sitePaneliWebView.As(&sitePaneliWebViewSurum3);
                sitePaneliWebViewSurum3->SetVirtualHostNameToFolderMapping(L"tarayici.local", varliklarKlasoru.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW); // Sahte güvenli adres ile gerçek klasör eşleştirilir

                // Paneldeki "Kaldır" tıklamaları ve kapatma isteği, toolbar'dakiyle AYNI komut işleyicisine gönderilir, komutların anlamı nereden geldiğine göre değişmez
                m_sitePaneliWebView->add_WebMessageReceived(
                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                        [this](ICoreWebView2* gonderen, ICoreWebView2WebMessageReceivedEventArgs* argumanlar) -> HRESULT
                        {
                            LPWSTR mesajMetni = nullptr;
                            if (SUCCEEDED(argumanlar->TryGetWebMessageAsString(&mesajMetni)))
                            {
                                ToolbarMesajiIsle(mesajMetni);
                                CoTaskMemFree(mesajMetni);
                            }
                            return S_OK;
                        }
                    ).Get(),
                    nullptr
                );

                // Kullanıcı panel açıkken başka bir yere tıklayıp odağı kaybettirince, gerçek bir açılır panel gibi otomatik olarak kapanır
                m_sitePaneliDenetleyici->add_LostFocus(
                    Callback<ICoreWebView2FocusChangedEventHandler>(
                        [this](ICoreWebView2Controller* gonderen, IUnknown* argumanlar) -> HRESULT
                        {
                            SiteBilgisiPaneliniKapat(); // Odak kaybedilince panel kapatılır
                            return S_OK;
                        }
                    ).Get(),
                    nullptr
                );

                m_sitePaneliWebView->Navigate(L"https://tarayici.local/site-paneli.html"); // Panelin HTML dosyası, sahte yerel adres üzerinden açılır
                return S_OK;
            }
        ).Get()
    );
}

// Bu fonksiyon, sertifika hatası bildirim panelini gösterecek AYRI denetleyiciyi oluşturur
void BrowserWindow::SertifikaWebViewOlustur()
{
    m_webViewOrtami->CreateCoreWebView2Controller(
        m_pencere,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this, gecerlilikBayragi = m_pencereGecerliMi](HRESULT sonuc, ICoreWebView2Controller* denetleyici) -> HRESULT
            {
                if (!*gecerlilikBayragi) { return S_OK; } // Pencere, bu denetleyici hazır olmadan önce kapatılmaya başladıysa, "this" işaretçisine hiç dokunulmadan güvenle vazgeçilir

                m_sertifikaDenetleyici = denetleyici; // Oluşan denetleyici, bu sınıfın sertifika denetleyici değişkenine kaydedilir
                m_sertifikaDenetleyici->get_CoreWebView2(&m_sertifikaWebView); // Denetleyicinin içindeki asıl WebView2 nesnesi alınır
                m_sertifikaDenetleyici->put_IsVisible(FALSE); // Panel, bir sertifika hatası gelene kadar görünmez tutulur

                std::wstring varliklarKlasoru = CalismaKlasoruBul() + L"\\assets"; // Panelin HTML dosyasının bulunduğu klasörün tam yolu

                ComPtr<ICoreWebView2_3> sertifikaWebViewSurum3; // SetVirtualHostNameToFolderMapping fonksiyonu daha yeni bir arayüz sürümünde bulunduğu için
                m_sertifikaWebView.As(&sertifikaWebViewSurum3);
                sertifikaWebViewSurum3->SetVirtualHostNameToFolderMapping(L"tarayici.local", varliklarKlasoru.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW); // Sahte güvenli adres ile gerçek klasör eşleştirilir

                // Paneldeki "Geri Dön"/"Yine de Git" tıklamaları, toolbar'dakiyle AYNI komut işleyicisine gönderilir
                m_sertifikaWebView->add_WebMessageReceived(
                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                        [this](ICoreWebView2* gonderen, ICoreWebView2WebMessageReceivedEventArgs* argumanlar) -> HRESULT
                        {
                            LPWSTR mesajMetni = nullptr;
                            if (SUCCEEDED(argumanlar->TryGetWebMessageAsString(&mesajMetni)))
                            {
                                ToolbarMesajiIsle(mesajMetni);
                                CoTaskMemFree(mesajMetni);
                            }
                            return S_OK;
                        }
                    ).Get(),
                    nullptr
                );

                m_sertifikaWebView->Navigate(L"https://tarayici.local/sertifika-hatasi.html"); // Panelin HTML dosyası, sahte yerel adres üzerinden açılır
                return S_OK;
            }
        ).Get()
    );
}

// Bu fonksiyon, bir sayfadaki şifre alanına tıklanınca beliren otomatik doldurma panelini gösterecek AYRI denetleyiciyi oluşturur
void BrowserWindow::DoldurmaWebViewOlustur()
{
    m_webViewOrtami->CreateCoreWebView2Controller(
        m_pencere,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this, gecerlilikBayragi = m_pencereGecerliMi](HRESULT sonuc, ICoreWebView2Controller* denetleyici) -> HRESULT
            {
                if (!*gecerlilikBayragi) { return S_OK; } // Pencere, bu denetleyici hazır olmadan önce kapatılmaya başladıysa, "this" işaretçisine hiç dokunulmadan güvenle vazgeçilir

                m_doldurmaDenetleyici = denetleyici; // Oluşan denetleyici, bu sınıfın doldurma denetleyici değişkenine kaydedilir
                m_doldurmaDenetleyici->get_CoreWebView2(&m_doldurmaWebView); // Denetleyicinin içindeki asıl WebView2 nesnesi alınır
                m_doldurmaDenetleyici->put_IsVisible(FALSE); // Panel, bir öneri gösterilene kadar görünmez tutulur

                std::wstring varliklarKlasoru = CalismaKlasoruBul() + L"\\assets"; // Panelin HTML dosyasının bulunduğu klasörün tam yolu

                ComPtr<ICoreWebView2_3> doldurmaWebViewSurum3; // SetVirtualHostNameToFolderMapping fonksiyonu daha yeni bir arayüz sürümünde bulunduğu için
                m_doldurmaWebView.As(&doldurmaWebViewSurum3);
                doldurmaWebViewSurum3->SetVirtualHostNameToFolderMapping(L"tarayici.local", varliklarKlasoru.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW); // Sahte güvenli adres ile gerçek klasör eşleştirilir

                // Paneldeki hesap önerisi tıklamaları ve kapatma isteği, kendine özel bir işleyiciye gönderilir, bu panel genel ToolbarMesajiIsle'ı KULLANMAZ çünkü komutları (Windows kimlik doğrulaması tetikleyen) kendine özeldir
                m_doldurmaWebView->add_WebMessageReceived(
                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                        [this](ICoreWebView2* gonderen, ICoreWebView2WebMessageReceivedEventArgs* argumanlar) -> HRESULT
                        {
                            LPWSTR mesajMetni = nullptr;
                            if (SUCCEEDED(argumanlar->TryGetWebMessageAsString(&mesajMetni)))
                            {
                                std::wstring mesaj = mesajMetni;
                                CoTaskMemFree(mesajMetni);
                                if (mesaj.rfind(L"autofill_choice|", 0) == 0) // Kullanıcı önerilen hesaplardan birine tıkladıysa
                                {
                                    OtomatikDoldurmaSecimiYap(mesaj.substr(16)); // Seçilen kullanıcı adıyla doldurma işlemi başlatılır
                                }
                                else if (mesaj == L"autofill_close") // Kullanıcı paneli kapattıysa (Escape veya dışarı tıklama)
                                {
                                    DoldurmaPaneliniKapat();
                                }
                            }
                            return S_OK;
                        }
                    ).Get(),
                    nullptr
                );

                // Kullanıcı panel açıkken başka bir yere tıklayıp odağı kaybettirince, otomatik olarak kapanır
                m_doldurmaDenetleyici->add_LostFocus(
                    Callback<ICoreWebView2FocusChangedEventHandler>(
                        [this](ICoreWebView2Controller* gonderen, IUnknown* argumanlar) -> HRESULT
                        {
                            DoldurmaPaneliniKapat();
                            return S_OK;
                        }
                    ).Get(),
                    nullptr
                );

                m_doldurmaWebView->Navigate(L"https://tarayici.local/doldurma-bildirimi.html"); // Panelin HTML dosyası, sahte yerel adres üzerinden açılır
                return S_OK;
            }
        ).Get()
    );
}

// Bu fonksiyon, adres çubuğu öneri panelini gösterecek AYRI denetleyiciyi oluşturur
void BrowserWindow::OneriWebViewOlustur()
{
    m_webViewOrtami->CreateCoreWebView2Controller(
        m_pencere,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this, gecerlilikBayragi = m_pencereGecerliMi](HRESULT sonuc, ICoreWebView2Controller* denetleyici) -> HRESULT
            {
                if (!*gecerlilikBayragi) { return S_OK; }

                m_oneriDenetleyici = denetleyici;
                m_oneriDenetleyici->get_CoreWebView2(&m_oneriWebView);
                m_oneriDenetleyici->put_IsVisible(FALSE); // Panel, bir öneri gösterilene kadar görünmez tutulur

                std::wstring varliklarKlasoru = CalismaKlasoruBul() + L"\\assets";
                ComPtr<ICoreWebView2_3> oneriWebViewSurum3;
                m_oneriWebView.As(&oneriWebViewSurum3);
                oneriWebViewSurum3->SetVirtualHostNameToFolderMapping(L"tarayici.local", varliklarKlasoru.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);

                // Paneldeki öneri tıklamaları, toolbar'dakiyle AYNI komut işleyicisine gönderilir
                m_oneriWebView->add_WebMessageReceived(
                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                        [this](ICoreWebView2* gonderen, ICoreWebView2WebMessageReceivedEventArgs* argumanlar) -> HRESULT
                        {
                            LPWSTR mesajMetni = nullptr;
                            if (SUCCEEDED(argumanlar->TryGetWebMessageAsString(&mesajMetni)))
                            {
                                ToolbarMesajiIsle(mesajMetni);
                                CoTaskMemFree(mesajMetni);
                            }
                            return S_OK;
                        }
                    ).Get(),
                    nullptr
                );

                // Kullanıcı panel açıkken başka bir yere (içeriğe, toolbar'a, pencere dışına) tıklayıp odağı kaçırınca, otomatik olarak kapanır
                // Tıklanan yer panelin KENDİSİ (bir öneri satırı) ise odak panelde kalır, bu yüzden bu olay hiç tetiklenmez ve tıklama güvenle işlenir
                m_oneriDenetleyici->add_LostFocus(
                    Callback<ICoreWebView2FocusChangedEventHandler>(
                        [this](ICoreWebView2Controller* gonderen, IUnknown* argumanlar) -> HRESULT
                        {
                            OneriPaneliniKapat();
                            return S_OK;
                        }
                    ).Get(),
                    nullptr
                );

                m_oneriWebView->Navigate(L"https://tarayici.local/adres-onerileri.html");
                return S_OK;
            }
        ).Get()
    );
}

// Bu fonksiyon, verilen adreste yeni bir sekme açar, yeni sekmenin kimlik numarasını hemen döndürür
int BrowserWindow::YeniSekmeOlustur(const std::wstring& baslangicAdresi, bool aktifYap, bool dahiliMi, bool gizliMi)
{
    if (m_rizaOnayiBekleniyor && baslangicAdresi != L"https://tarayici.local/onboarding.html") // Rıza onayı verilmeden, karşılama sekmesi dışında hiçbir sekme açılamaz
    {
        m_secimSekmeId = m_onboardingSekmeId; // Yeni sekme açılmaz, kullanıcı mevcut karşılama sekmesine geri döndürülür
        SekmeListesiniGonder();
        return m_onboardingSekmeId;
    }

    int yeniId = m_sonrakiSekmeId; // Bu yeni sekmeye verilecek kimlik numarası, bir sonraki kullanılabilir numara olarak belirlenir
    m_sonrakiSekmeId++; // Bir sonraki sekme için kullanılacak numara bir artırılır

    // Gerçek WebView2 denetleyicisi henüz hiç oluşturulmadan, bu sekmeyi temsil eden boş bir yer tutucu nesne hemen hazırlanıp listeye eklenir
    // Böylece kullanıcı artık denetleyicinin hazırlanmasını (yüz milisaniyeden birkaç saniyeye kadar sürebilen bir süreci) hiç beklemeden, sekme şeridinde yeni sekmeyi ANINDA görür
    auto yerTutucuSekme = std::make_unique<SekmeBilgisi>(); // Denetleyicisi ve webView'i henüz boş olan geçici sekme nesnesi
    yerTutucuSekme->id = yeniId; // Kimlik numarası, denetleyici hazır olduğunda bu sekmeyi tekrar bulabilmek için atanır
    yerTutucuSekme->baslik = L"Yükleniyor..."; // Denetleyici hazır olup gerçek sayfa başlığı gelene kadar şeritte gösterilecek geçici başlık
    yerTutucuSekme->yukleniyor = true; // Henüz hiçbir şey yüklenmemiş olsa da, kullanıcıya bu sekmenin hazırlanmakta olduğu gösterilir
    yerTutucuSekme->dahiliSayfaMi = dahiliMi; // Dahili/normal ayrımı daha en baştan, denetleyici hazır olmadan bile bilinir
    yerTutucuSekme->gizliMi = gizliMi; // Gizli sekme ayrımı da daha en baştan bilinir
    yerTutucuSekme->sonAktifZaman = GetTickCount64(); // Oluşturulma anı, şimdilik son aktif zaman olarak kaydedilir
    m_sekmeler.push_back(std::move(yerTutucuSekme)); // Yer tutucu sekme, denetleyicisi olmadan sekme listesine eklenir

    if (aktifYap) // Eğer bu yeni sekme kullanıcı tarafından hemen görülmek isteniyorsa
    {
        // Sekme şeridinde seçili görünmesi gereken sekme olarak hemen işaretlenir, ama EKRANDA GÖSTERİLEN içerik (m_aktifSekmeId) bilerek hemen değiştirilmez
        // Eski sekmenin görünümü, bu yeni sekmenin denetleyicisi hazır olup SekmeDenetleyicisiHazirlandi tarafından gerçek geçiş yapılana kadar ekranda kalmaya devam eder, böylece kullanıcı asla boş veya çökmüş bir pencere görmez
        m_secimSekmeId = yeniId;
    }

    SekmeListesiniGonder(); // Yeni sekme, denetleyicisi hiç hazırlanmadan, şimdiden sekme şeridinde görünür hale gelir

    if (!m_yedekler.empty()) // Eğer arka planda önceden hazırlanmış, havuzda kullanıma hazır en az bir yedek denetleyici varsa
    {
        Microsoft::WRL::ComPtr<ICoreWebView2Controller> hazirDenetleyici = m_yedekler.back().denetleyici; // Havuzdaki son yedek, kaybolmaması için önce yerel bir değişkene alınır
        m_yedekler.pop_back(); // Bu yedek, havuzdan çıkarılır, artık kullanıma alınmıştır

        bool zatenYuklenmisMi = (baslangicAdresi == m_yedekAdresi); // Yedek, zaten tam olarak istenen adrese önceden gezindirilmişse, pahalı bir tekrar gezinmeye hiç gerek yoktur
        SekmeDenetleyicisiHazirlandi(hazirDenetleyici.Get(), yeniId, baslangicAdresi, aktifYap, dahiliMi, gizliMi, zatenYuklenmisMi); // Denetleyici zaten var olduğu için, yeni bir WebView2 süreci beklenmeden sekme neredeyse anında hazır olur
        YedekSekmeHazirla(); // Havuzdaki eksilen yer için, arka planda yeni bir yedek hazırlanmaya başlanır, bu sayede art arda hızlıca açılan birkaç sekme de anında açılabilir

        return yeniId; // Sekmenin kimlik numarası döndürülür
    }

    // Havuzda hazır bir yedek yoksa (örneğin program henüz yeni açıldıysa, ya da havuzun kaldırabileceğinden daha hızlı art arda birden fazla sekme açıldıysa), eskisi gibi sıfırdan bir denetleyici oluşturulur
    m_webViewOrtami->CreateCoreWebView2Controller(
        m_pencere,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this, yeniId, baslangicAdresi, aktifYap, dahiliMi, gizliMi](HRESULT sonuc, ICoreWebView2Controller* denetleyici) -> HRESULT
            {
                SekmeDenetleyicisiHazirlandi(denetleyici, yeniId, baslangicAdresi, aktifYap, dahiliMi, gizliMi, false); // Oluşan denetleyici, gerçek bir sekmeye dönüştürülür, sıfırdan oluşturulduğu için hiçbir adrese önceden gezindirilmemiştir
                YedekSekmeHazirla(); // Bu sekme de hazır olduğuna göre, bir dahaki yeni sekme isteği için şimdiden bir yedek hazırlanmaya başlanır
                return S_OK; // İşlem başarıyla ele alındı
            }
        ).Get()
    );

    return yeniId; // Henüz asenkron oluşturma tamamlanmamış olsa da, bu sekmenin kimlik numarası hemen döndürülür, çağıran taraf bu numarayı saklayıp daha sonra kullanabilir
}

// Bu fonksiyon, hazır bir WebView2 denetleyicisini (ister az önce sıfırdan oluşturulmuş, ister önceden hazırlanmış bir yedek olsun) alıp, YeniSekmeOlustur tarafından daha önce şeride eklenmiş yer tutucu sekmeye bağlar
void BrowserWindow::SekmeDenetleyicisiHazirlandi(ICoreWebView2Controller* denetleyici, int yeniId, const std::wstring& baslangicAdresi, bool aktifYap, bool dahiliMi, bool gizliMi, bool zatenYuklenmisMi)
{
    SekmeBilgisi* sekme = SekmeBul(yeniId); // Bu kimlik numarasıyla, YeniSekmeOlustur içinde daha önce şeride eklenmiş yer tutucu sekme aranır
    if (sekme == nullptr) // Kullanıcı, bu denetleyici daha hazır olmadan (örneğin çok hızlı art arda yeni sekme açıp hemen kapatarak) bu sekmeyi zaten kapatmış olabilir
    {
        denetleyici->put_IsVisible(FALSE); // Her ihtimale karşı, hiçbir sekmeye bağlanamayacak bu denetleyici önce görünmez yapılır
        denetleyici->Close(); // Artık bağlanacağı bir sekme kalmadığı için, boşuna RAM tutmaması için kapatılır
        return; // Yapılacak başka bir şey yoktur
    }

    sekme->denetleyici = denetleyici; // Verilen denetleyici, yer tutucu sekmeye artık gerçekten bağlanır
    sekme->denetleyici->get_CoreWebView2(&sekme->webView); // Denetleyicinin içindeki asıl WebView2 nesnesi alınır
    sekme->webView.As(&sekme->webViewGenisletilmis); // SetVirtualHostNameToFolderMapping, TrySuspend ve DownloadStarting gibi daha yeni özelliklere erişmek için genişletilmiş arayüze geçiş yapılır
    sekme->sonAktifZaman = GetTickCount64(); // Denetleyicinin bağlandığı an, en son aktif olduğu zaman olarak güncellenir

    { // Ses çalma durumu değişiklikleri dinlenir, sekme şeridinde ses ikonu gösterebilmek için
        ComPtr<ICoreWebView2_8> webViewSurum8;
        sekme->webView.As(&webViewSurum8);
        if (webViewSurum8 != nullptr)
        {
            webViewSurum8->add_IsDocumentPlayingAudioChanged(
                Callback<ICoreWebView2IsDocumentPlayingAudioChangedEventHandler>(
                    [this, yeniId](ICoreWebView2* gonderen, IUnknown* argumanlar) -> HRESULT
                    {
                        SekmeBilgisi* guncelSekme = SekmeBul(yeniId);
                        if (guncelSekme == nullptr) { return S_OK; }
                        ComPtr<ICoreWebView2_8> gonderenSurum8;
                        ComPtr<ICoreWebView2> gonderenPtr(gonderen);
                        gonderenPtr.As(&gonderenSurum8);
                        if (gonderenSurum8 != nullptr)
                        {
                            BOOL caliyorMu = FALSE;
                            gonderenSurum8->get_IsDocumentPlayingAudio(&caliyorMu);
                            guncelSekme->sesCaliyorMu = (caliyorMu == TRUE);
                            SekmeListesiniGonder(); // Sekme şeridinde ses ikonunun görünmesi için liste tekrar gönderilir
                        }
                        return S_OK;
                    }
                ).Get(),
                nullptr
            );
        }
    }
    { // Reklam/izleyici engelleme: her kaynak isteği (resim, script, iframe, vb.) bu olayla yakalanır, bilinen reklam/izleyici domainleriyle eşleşenler boş bir yanıtla engellenir
        sekme->webView->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
        sekme->webView->add_WebResourceRequested(
            Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                [this](ICoreWebView2* gonderen, ICoreWebView2WebResourceRequestedEventArgs* argumanlar) -> HRESULT
                {
                    if (!m_reklamEngellemeAcikMi && m_izleyiciEngellemeSeviyesi == L"kapali") { return S_OK; } // İkisi de kapalıysa, hiçbir isteği incelemeye bile gerek yoktur

                    ComPtr<ICoreWebView2WebResourceRequest> istek;
                    argumanlar->get_Request(&istek);
                    if (istek == nullptr) { return S_OK; }

                    LPWSTR uriPtr = nullptr;
                    istek->get_Uri(&uriPtr);
                    std::wstring uri = uriPtr ? uriPtr : L"";
                    if (uriPtr != nullptr) { CoTaskMemFree(uriPtr); }
                    if (uri.empty()) { return S_OK; }

                    bool engellenmeliMi = (m_reklamEngellemeAcikMi && Engelleyici::ReklamAdresiMi(uri)) || Engelleyici::IzleyiciAdresiMi(uri, m_izleyiciEngellemeSeviyesi);
                    if (!engellenmeliMi) { return S_OK; }

                    if (m_webViewOrtami != nullptr)
                    {
                        ComPtr<ICoreWebView2WebResourceResponse> bosYanit;
                        m_webViewOrtami->CreateWebResourceResponse(nullptr, 403, L"Blocked", L"", &bosYanit); // İçeriksiz, 403 durum kodlu bir yanıt; tarayıcıya "bu istek reddedildi" der, sayfanın geri kalanı normal yüklenmeye devam eder
                        argumanlar->put_Response(bosYanit.Get());
                    }
                    return S_OK;
                }
            ).Get(),
            nullptr
        );
    }

    { // Geçerli (hatasız) HTTPS bağlantılarında kilit ikonu panelinde gösterilecek sertifika detayları, Chrome DevTools Protokolü üzerinden yakalanır, çünkü WebView2'nin kendi ServerCertificateErrorDetected olayı sadece HATALI sertifikalarda tetiklenir
        sekme->webView->CallDevToolsProtocolMethod(L"Network.enable", L"{}", nullptr);
        ComPtr<ICoreWebView2DevToolsProtocolEventReceiver> cdpAlici;
        sekme->webView->GetDevToolsProtocolEventReceiver(L"Network.responseReceived", &cdpAlici);
        if (cdpAlici != nullptr)
        {
            cdpAlici->add_DevToolsProtocolEventReceived(
                Callback<ICoreWebView2DevToolsProtocolEventReceivedEventHandler>(
                    [this, yeniId](ICoreWebView2* gonderen, ICoreWebView2DevToolsProtocolEventReceivedEventArgs* argumanlar) -> HRESULT
                    {
                        SekmeBilgisi* guncelSekme = SekmeBul(yeniId);
                        if (guncelSekme == nullptr) { return S_OK; }

                        LPWSTR jsonPtr = nullptr;
                        argumanlar->get_ParameterObjectAsJson(&jsonPtr);
                        std::wstring json = jsonPtr ? jsonPtr : L"";
                        if (jsonPtr != nullptr) { CoTaskMemFree(jsonPtr); }

                        // Sadece ana belge yanıtının (iframe'ler veya alt kaynaklar değil) sertifikası ilgi alanımızdır
                        if (json.find(L"\"type\":\"Document\"") == std::wstring::npos) { return S_OK; }
                        size_t guvenlikKonumu = json.find(L"\"securityDetails\"");
                        if (guvenlikKonumu == std::wstring::npos) { return S_OK; } // Düz HTTP bağlantılarında bu alan hiç yoktur

                        std::wstring guvenlikMetni = json.substr(guvenlikKonumu); // securityDetails'ten sonrası, alanlarını JsonAlaniBul ile bulmak için yeterlidir
                        guncelSekme->sertifikaKonu = JsonAlaniBul(guvenlikMetni, L"subjectName");
                        guncelSekme->sertifikaYayinlayan = JsonAlaniBul(guvenlikMetni, L"issuer");
                        guncelSekme->sertifikaProtokol = JsonAlaniBul(guvenlikMetni, L"protocol");
                        if (m_sitePaneliAcikMi && guncelSekme->id == m_aktifSekmeId) { SiteBilgisiPaneliVeriGonder(); } // Panel bu sekme için zaten açıksa, sertifika az önce geldiği için hemen güncellenir
                        return S_OK;
                    }
                ).Get(),
                nullptr
            );
        }
    }

    sekme->denetleyici->put_IsVisible(FALSE); // Sekme henüz ekrana alınmaya karar verilmediği için (aşağıda, hazırsa) başlangıçta gizli tutulur

    if (dahiliMi) // Eğer bu sekme, programın kendi dahili bir sayfasını (örneğin Ayarlar) gösterecekse
    {
        std::wstring varliklarKlasoru = CalismaKlasoruBul() + L"\\assets"; // Dahili sayfaların HTML dosyalarının bulunduğu klasörün tam yolu hesaplanır
        sekme->webViewGenisletilmis->SetVirtualHostNameToFolderMapping(L"tarayici.local", varliklarKlasoru.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW); // Sahte güvenli adres ile gerçek klasör eşleştirilir, SADECE dahili sekmelerde yapılır, normal internet siteleri bu adrese asla erişemez

        std::wstring resimlerKlasoru = ResimlerKlasorunuBul(); // Ayarlar sayfasındaki arka plan galerisinin, fotoğrafları Base64'e çevirmeden doğrudan <img> ile gösterebilmesi için "resimler" klasörü de ayrı bir sahte adrese eşlenir
        if (!resimlerKlasoru.empty())
        {
            sekme->webViewGenisletilmis->SetVirtualHostNameToFolderMapping(L"tarayici-resimler.local", resimlerKlasoru.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
        }

        // Dahili sayfadan gelen mesajlar, normal sitelerden farklı, ayrıcalıklı bir işleyiciye yönlendirilir, bu işleyici asla normal bir internet sitesine bağlanmaz
        sekme->webView->add_WebMessageReceived(
            Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                [this, yeniId](ICoreWebView2* gonderen, ICoreWebView2WebMessageReceivedEventArgs* argumanlar) -> HRESULT
                {
                    // Güvenlik için çok önemli bir kontrol: bu sekme dahili olarak oluşturulmuş olsa bile, kullanıcı daha sonra bu sekmede sıradan bir internet sitesine gitmiş olabilir (örneğin hızlı erişim sayfasındaki bir siteye tıklayarak), bu yüzden mesajın GERÇEKTEN tarayici.local adresinden geldiği, her mesajda yeniden doğrulanır
                    LPWSTR kaynakAdres = nullptr; // Mesajı gönderen sayfanın adresini tutacak değişken
                    argumanlar->get_Source(&kaynakAdres); // Mesajın geldiği gerçek adres okunur
                    bool guvenilirKaynak = kaynakAdres != nullptr && wcsncmp(kaynakAdres, L"https://tarayici.local/", 23) == 0; // Adres gerçekten bizim dahili sayfalarımıza mı ait kontrol edilir
                    if (kaynakAdres != nullptr) { CoTaskMemFree(kaynakAdres); } // Ayrılan bellek serbest bırakılır

                    if (!guvenilirKaynak) // Eğer mesaj güvenilir olmayan bir kaynaktan (sıradan bir internet sitesinden) geldiyse
                    {
                        return S_OK; // Mesaj tamamen yok sayılır, hiçbir ayrıcalıklı komut çalıştırılmaz
                    }

                    LPWSTR mesajMetni = nullptr; // Gelen mesajın düz metin halini tutacak değişken
                    if (SUCCEEDED(argumanlar->TryGetWebMessageAsString(&mesajMetni))) // Mesaj düz bir metin olarak başarıyla okunabildiyse
                    {
                        DahiliSayfaMesajiIsle(yeniId, mesajMetni); // Mesaj, dahili sayfalara özel işleyiciye gönderilir
                        CoTaskMemFree(mesajMetni); // Ayrılan bellek serbest bırakılır
                    }
                    return S_OK; // Mesaj başarıyla ele alındı
                }
            ).Get(),
            nullptr
        );
    }
    else // Eğer bu sekme, normal bir internet sitesi gösterecekse
    {
        // Normal sitelerden gelen mesajlar, dahili sayfalardakinden TAMAMEN farklı, çok sınırlı bir işleyiciye yönlendirilir, bu işleyici SADECE otomatik doldurma ile ilgili iki komutu tanır, başka hiçbir şeyi (ayarlar, şifreler, izinler gibi) kabul etmez
        sekme->webView->add_WebMessageReceived(
            Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                [this, yeniId](ICoreWebView2* gonderen, ICoreWebView2WebMessageReceivedEventArgs* argumanlar) -> HRESULT
                {
                    // Hangi siteden geldiği, sayfanın mesaj içine koyduğu bir veriye göre DEĞİL, WebView2'nin kendi bildirdiği gerçek kaynak adresine göre belirlenir, böylece bir sayfa başka bir sitenin host adını taklit edemez
                    LPWSTR kaynakAdres = nullptr;
                    argumanlar->get_Source(&kaynakAdres);
                    std::wstring gercekKaynak = kaynakAdres ? kaynakAdres : L"";
                    if (kaynakAdres != nullptr) { CoTaskMemFree(kaynakAdres); }

                    LPWSTR mesajMetni = nullptr;
                    if (SUCCEEDED(argumanlar->TryGetWebMessageAsString(&mesajMetni)))
                    {
                        OtomatikDoldurmaIsteginiIsle(yeniId, gercekKaynak, mesajMetni); // Mesaj, SADECE otomatik doldurma isteklerini tanıyan kendi özel işleyicisine gönderilir
                        CoTaskMemFree(mesajMetni);
                    }
                    return S_OK;
                }
            ).Get(),
            nullptr
        );

        // Her yeni belge (sayfa) oluşturulduğunda, sayfanın kendi JavaScript'i çalışmaya başlamadan ÖNCE bu küçük betik çalıştırılır
        // Betik, sayfada herhangi bir şifre alanına odaklanıldığında bunu fark eder ve native tarafa (sadece ekran koordinatlarıyla, host bilgisi vermeden) haber verir, kayıtlı hesap olup olmadığına native taraf kendi bildiği gerçek kaynağa göre karar verir
        static const wchar_t* otomatikDoldurmaBetigi =
            L"(function(){"
            L"function alanaOdaklanildi(e){"
            L"var r=e.target.getBoundingClientRect();"
            L"window.chrome.webview.postMessage('autofill_check|'+JSON.stringify({top:r.bottom,left:r.left,width:r.width}));"
            L"window.__tarayiciSonSifreAlani=e.target;"
            L"}"
            L"function tara(){document.querySelectorAll('input[type=password]').forEach(function(girdi){"
            L"if(girdi.dataset.tarayiciAutofillBagli)return;girdi.dataset.tarayiciAutofillBagli='1';"
            L"girdi.addEventListener('focus',alanaOdaklanildi);"
            L"});}"
            L"function kullaniciAlaniniBul(sifreAlani){"
            L"var form=sifreAlani.closest('form');var kapsayici=form||document;"
            L"var girdiler=Array.from(kapsayici.querySelectorAll('input'));var i=girdiler.indexOf(sifreAlani);"
            L"for(var k=i-1;k>=0;k--){var tip=(girdiler[k].type||'text').toLowerCase();if(tip==='text'||tip==='email'||tip==='tel')return girdiler[k];}"
            L"return null;"
            L"}"
            L"window.chrome.webview.addEventListener('message',function(e){"
            L"var veri;try{veri=JSON.parse(e.data);}catch(hata){return;}"
            L"if(veri.type==='autofill_fill'&&window.__tarayiciSonSifreAlani){"
            L"var sifreAlani=window.__tarayiciSonSifreAlani;"
            L"var ayarlayici=Object.getOwnPropertyDescriptor(window.HTMLInputElement.prototype,'value').set;"
            L"ayarlayici.call(sifreAlani,veri.password);sifreAlani.dispatchEvent(new Event('input',{bubbles:true}));"
            L"var kullaniciAlani=kullaniciAlaniniBul(sifreAlani);"
            L"if(kullaniciAlani){ayarlayici.call(kullaniciAlani,veri.username);kullaniciAlani.dispatchEvent(new Event('input',{bubbles:true}));}"
            L"}"
            L"});"
            L"if(document.readyState!=='loading'){tara();}else{document.addEventListener('DOMContentLoaded',tara);}"
            L"new MutationObserver(tara).observe(document.documentElement||document,{childList:true,subtree:true});"
            L"})();";
        sekme->webView->AddScriptToExecuteOnDocumentCreated(
            otomatikDoldurmaBetigi,
            Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                [](HRESULT sonuc, LPCWSTR betikId) -> HRESULT { return S_OK; }
            ).Get()
        );

        // Kullanıcının Ayarlar sayfasından eklediği, etkin olan kullanıcı betikleri (userscript), bu sekmenin yükleyeceği her sayfada çalıştırılmak üzere hazırlanır
        // Her betiğin deseni basit bir joker karakter (*) eşleşmesiyle kontrol edilir, eşleşmezse o betik hiç çalıştırılmaz
        std::vector<KullaniciBetigi> betikler = BetikKasasi::BetikleriOku();
        std::wstring betiklerJson = L"["; // Her betiğin deseni ve kodu, sayfanın kendi JavaScript'ine bir veri dizisi olarak gömülür
        bool ilkBetikMi = true;
        for (const KullaniciBetigi& betik : betikler)
        {
            if (!betik.etkinMi) { continue; } // Kapatılmış betikler hiç gömülmez
            if (!ilkBetikMi) { betiklerJson += L","; }
            betiklerJson += L"{\"desen\":\"" + JsonIcinKacisla(betik.desen) + L"\",\"kod\":\"" + JsonIcinKacisla(betik.kod) + L"\"}";
            ilkBetikMi = false;
        }
        betiklerJson += L"]";

        if (!betikler.empty()) // Hiç kayıtlı betik yoksa, boşuna bir betik enjekte edilmez
        {
            std::wstring betikCalistiriciBetigi = L"(function(){"
                L"var betikler=" + betiklerJson + L";"
                L"function desenEslesiyorMu(desen){var parcalar=desen.split('*');var index=0;var metin=location.href;for(var i=0;i<parcalar.length;i++){if(parcalar[i]===''){continue;}var bulunan=metin.indexOf(parcalar[i],index);if(bulunan===-1){return false;}index=bulunan+parcalar[i].length;}return true;}"
                L"betikler.forEach(function(b){try{if(desenEslesiyorMu(b.desen)){(new Function(b.kod))();}}catch(e){}});"
                L"})();";

            sekme->webView->AddScriptToExecuteOnDocumentCreated(
                betikCalistiriciBetigi.c_str(),
                Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                    [](HRESULT sonuc, LPCWSTR betikId) -> HRESULT { return S_OK; }
                ).Get()
            );
        }
    }

    SekmeOlaylariniBagla(yeniId); // Bu sekmenin sayfa geçişi, başlık değişimi, çökme ve indirme olaylarını dinleyen işleyiciler bağlanır

    if (zatenYuklenmisMi) // Bu denetleyici, bu adrese arka planda zaten önceden gezindirilmişse, tekrar gezinmek (sayfayı baştan yükleyip betiği yeniden çalıştırmak) gereksiz bir gecikmeye sebep olur
    {
        // Bu denetleyici artık boşta bekleyen bir yedek değil, gerçekten kullanıcının gördüğü aktif bir sekme olduğu için, boştayken istenen düşük bellek hedefi geri alınır
        ComPtr<ICoreWebView2_19> webViewSurum19; // MemoryUsageTargetLevel özelliği için daha yeni arayüz sürümüne geçiş yapılır
        sekme->webView.As(&webViewSurum19); // Dönüşüm yapılır
        if (webViewSurum19) // Dönüşüm başarılıysa
        {
            webViewSurum19->put_MemoryUsageTargetLevel(COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_NORMAL); // Artık aktif kullanımda olduğu için normal bellek hedefine dönülür
        }

        sekme->yukleniyor = false; // Yedek zaten önceden gezindirilip yüklendiği için, bu sekme artık "Yükleniyor..." durumunda kalmamalıdır
        LPWSTR mevcutBaslik = nullptr; // Yedeğin arka planda zaten yüklemiş olduğu sayfanın gerçek başlığını tutacak değişken
        sekme->webView->get_DocumentTitle(&mevcutBaslik); // Güncel başlık okunur
        if (mevcutBaslik != nullptr) // Eğer başarıyla okunabildiyse
        {
            sekme->baslik = mevcutBaslik; // Sekmenin başlığı, yer tutucudaki "Yükleniyor..." yazısı yerine gerçek sayfa başlığıyla güncellenir
            CoTaskMemFree(mevcutBaslik); // Ayrılan bellek serbest bırakılır
        }

        // Sayfa daha ilk kez yüklenirken (henüz bu sekmeye bağlanmadan önce) kendi ihtiyaç duyduğu verileri (hızlı erişim listesi, tema) istemişti, ama o an dinleyici henüz bağlı olmadığı için bu istekler cevapsız kalmıştı
        // Bu yüzden o cevaplar, sayfayı yeniden yüklemeye gerek kalmadan, şimdi doğrudan gönderilir
        AyarVerisi gunceAyarlar = Ayarlar::Oku(); // Güncel ayarlar diskten okunur
        SekmeyeMesajGonder(yeniId, L"{\"type\":\"theme\",\"theme\":" + TemaNesnesiJsonYap(gunceAyarlar) + L"}"); // Tema bilgisi gönderilir
        std::vector<SayfaKaydi> enCokZiyaretEdilenler = EnCokZiyaretEdilenler(8); // Geçmişten, en çok ziyaret edilen en fazla sekiz site hesaplanır
        SekmeyeMesajGonder(yeniId, L"{\"type\":\"topsites\",\"entries\":" + SayfaKayitlariniJsonYap(enCokZiyaretEdilenler) + L"}"); // Hızlı erişim listesi gönderilir
        YeniSekmeArkaPlaniniGonder(yeniId); // Arka plan resmi isteği de aynı sebepten cevapsız kalmıştı, bu da burada yeniden gönderilir, aksi halde sekme hiçbir zaman resim almayıp CSS'teki sabit varsayılan arka plana düşer
    }
    else // Aksi halde, bu sekme daha önce hiç bu adrese gezinmemiştir, normal şekilde gezinilir
    {
        sekme->webView->Navigate(baslangicAdresi.c_str()); // Bu sekmede, verilen başlangıç adresi açılır
    }

    if (aktifYap && m_secimSekmeId == yeniId) // Kullanıcı bu sekmeyi istediği andan bu yana araya başka bir sekmeye geçmediyse, denetleyici artık hazır olduğuna göre gerçek görsel geçiş güvenle yapılabilir
    {
        SekmeyeGec(yeniId); // Bu sekme artık gerçekten ekrana alınır, SekmeyeGec kendi içinde zaten güncel sekme listesini de gönderir
    }
    else // Kullanıcı bu arada başka bir sekmeye geçmişse, ya da bu sekme zaten aktif yapılmak istenmemişse
    {
        SekmeListesiniGonder(); // Ekrana alınmasa bile, başlık ve yükleniyor durumu gibi değişikliklerin şeritte görünmesi için liste yine de gönderilir
    }
}

// Bu fonksiyon, arka planda, görünmez, hiçbir sekmeye bağlı olmayan bir veya daha fazla yedek WebView2 denetleyicisi oluşturup havuza ekler
// Yeni bir ICoreWebView2Controller oluşturmak (kendi süreç ve bileşenleriyle) gözle görülür bir gecikmeye sebep olur, bu fonksiyon sayesinde bu gecikme, kullanıcı "+" butonuna basmadan ÖNCE, arka planda sessizce yaşanmış olur, kullanıcı tıkladığında yedek anında kullanılır
// Havuzda birden fazla yedek tutulmasının sebebi, kullanıcı art arda hızlıca birkaç sekme açtığında (tek yedekle sadece İLK sekme anında açılır) bunların HEPSİNİN anında açılabilmesidir
// Havuzun hedef boyutuna ULAŞMASI gereken eksik yedek sayısının TAMAMI, burada tek tek sırayla değil, AYNI ANDA paralel olarak oluşturulmaya başlanır, aksi halde (örneğin program yeni açıldığında) havuz tek tek doldurulsaydı, ikinci veya üçüncü sekme isteği hâlâ boşta kalan yedek yokken gelip yavaş kalabilirdi
void BrowserWindow::YedekSekmeHazirla()
{
    if (!m_webViewOrtami) // Eğer WebView2 ortamı henüz hazır değilse (programın en başındaki çok erken bir çağrıysa)
    {
        return; // Ortam hazır olmadan denetleyici oluşturulamaz, bu fonksiyon ortam hazır olduktan sonra zaten tekrar çağrılacaktır
    }

    // Havuzda zaten hazır olan yedekler ile şu an paralel olarak hazırlanmakta olan yedekler toplamı, hedef boyuta ulaşana kadar yeni oluşturma istekleri başlatılır
    while (m_yedekler.size() + m_yedekHazirlananSayisi < YedekHavuzuHedefBoyutu)
    {
        m_yedekHazirlananSayisi++; // Paralel hazırlanan yedek sayısı bir artırılır, bu sayede bu döngü bir dahaki turunda doğru eksik miktarı hesaplar

        m_webViewOrtami->CreateCoreWebView2Controller(
            m_pencere,
            Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [this](HRESULT sonuc, ICoreWebView2Controller* denetleyici) -> HRESULT
                {
                    // DİKKAT: m_yedekHazirlananSayisi burada DEĞİL, ya aşağıdaki başarısızlık dallarında ya da gezinme GERÇEKTEN tamamlandığında (NavigationCompleted içinde) azaltılır
                    // Çünkü denetleyicinin oluşması, hazırlığın sadece ilk yarısıdır, asıl "hazır" sayılması gereken an, ön ısıtma gezinmesinin de bitmiş olduğu andır
                    if (!SUCCEEDED(sonuc) || denetleyici == nullptr) // Denetleyici oluşturulamadıysa
                    {
                        m_yedekHazirlananSayisi--; // Hazırlanmakta sayılmaktan çıkar, bu deneme başarısız oldu
                        return S_OK; // Yapılacak başka bir şey yoktur
                    }

                    denetleyici->put_IsVisible(FALSE); // Henüz hiçbir sekmeye bağlı olmadığı için, görünmez tutulur

                    // Yedek, henüz görünmez olsa bile, GERÇEK pencere boyutuna şimdiden ayarlanır
                    // Aksi halde yedek 0x0 boyutta boşta bekler, sekmeye bağlanıp görünür yapıldığı anda WebView2 ilk kez gerçek boyutta bir kare çizmek zorunda kalır, bu da kullanıcıya küçük bir "takılma" hissi verir
                    // Boyut burada önceden verilince, sekmeye bağlanma anında sadece görünürlük değişir, ekstra bir boyutlandırma işi çıkmaz
                    RECT gecerliIstemciAlani; // Ana pencerenin güncel iç alanını tutacak dikdörtgen
                    GetClientRect(m_pencere, &gecerliIstemciAlani); // Pencerenin güncel iç alanı okunur
                    RECT yedekIcerikAlani = { 0, ToplamToolbarYuksekligi(), gecerliIstemciAlani.right, gecerliIstemciAlani.bottom }; // Gerçek bir aktif sekmenin kaplayacağı alanın aynısı
                    denetleyici->put_Bounds(yedekIcerikAlani); // Yedek, görünmeden önce bu gerçek boyuta ayarlanır

                    // Denetleyicinin kendisini oluşturmak, gecikmenin sadece bir kısmıdır, asıl büyük gecikme bu denetleyicinin İLK kez bir internet sitesi veya dahili sayfa gezinmesi yapması sırasında (o adresin arka plan süreci ilk kez başlarken) yaşanır
                    // Bu yüzden yedek daha hiçbir sekmeye bağlanmadan, arka planda şimdiden hızlı erişim sayfasına gezindirilir, bu sayede o ağır ilk gezinme maliyeti kullanıcı "+" butonuna basmadan ÖNCE, fark edilmeden ödenmiş olur
                    ComPtr<ICoreWebView2> onIsitmaWebView; // Ön ısıtma gezinmesi için geçici bir WebView2 işaretçisi
                    denetleyici->get_CoreWebView2(&onIsitmaWebView); // Denetleyicinin içindeki asıl WebView2 nesnesi alınır
                    if (!onIsitmaWebView) // Eğer bir sebepten alınamadıysa
                    {
                        m_yedekHazirlananSayisi--; // Hazırlanmakta sayılmaktan çıkar, bu denetleyici kullanılamaz durumda
                        return S_OK; // Bu nadir durumda yedek sessizce atlanır
                    }

                    ComPtr<ICoreWebView2_3> onIsitmaWebViewSurum3; // SetVirtualHostNameToFolderMapping için daha yeni arayüz sürümüne geçiş yapılır
                    onIsitmaWebView.As(&onIsitmaWebViewSurum3); // Dönüşüm yapılır
                    if (onIsitmaWebViewSurum3) // Dönüşüm başarılıysa
                    {
                        std::wstring varliklarKlasoru = CalismaKlasoruBul() + L"\\assets"; // Dahili sayfaların bulunduğu klasörün tam yolu
                        onIsitmaWebViewSurum3->SetVirtualHostNameToFolderMapping(L"tarayici.local", varliklarKlasoru.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW); // Sahte güvenli adres, gerçek klasörle eşleştirilir
                    }

                    // Yedek, kullanılmayı bekleyen boşta bir denetleyici olduğu için, WebView2'ye bu süreçten mümkün olduğunca az bellek tutmasını söyleyebiliriz
                    // Bu, TrySuspend'den farklıdır, süreci tamamen askıya almaz (bu yüzden kullanıcı tıklayınca uyandırma gecikmesi yaşanmaz), sadece önbellekleri ve kullanılmayan belleği daha agresif şekilde geri vermesini ister
                    ComPtr<ICoreWebView2_19> onIsitmaWebViewSurum19; // MemoryUsageTargetLevel özelliği için daha yeni arayüz sürümüne geçiş yapılır
                    onIsitmaWebView.As(&onIsitmaWebViewSurum19); // Dönüşüm yapılır
                    if (onIsitmaWebViewSurum19) // Dönüşüm başarılıysa (yüklü WebView2 Runtime bu kadar yeniyse)
                    {
                        onIsitmaWebViewSurum19->put_MemoryUsageTargetLevel(COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_LOW); // Yedek boşta beklerken düşük bellek hedefi istenir
                    }

                    // ÖNEMLİ: yedek, Navigate() çağrıldığı AN değil, bu gezinme GERÇEKTEN tamamlandığında (NavigationCompleted ateşlendiğinde) havuza eklenir
                    // Çünkü Navigate() asenkron bir istektir, hemen havuza eklenseydi, kullanıcı bu yedeği gezinme bitmeden kullanıma alabilir, bu da adres çubuğunda bir an "about:blank" görünüp içeriğin geç gelmesine sebep olurdu
                    // Dinleyici sadece bu TEK ön ısıtma gezinmesi için var olmalı, bu yüzden kendi kendini (remove_NavigationCompleted ile) hemen kaldırır, aksi halde bu denetleyici gerçek bir sekmeye dönüştükten sonra yapacağı HER gezinmede yanlışlıkla tekrar havuza eklenmeye çalışılırdı
                    ComPtr<ICoreWebView2Controller> denetleyiciPaylasilan = denetleyici; // Havuza eklenirken kullanılmak üzere, denetleyicinin ömrünü uzatan paylaşılan bir referans
                    auto yedekGezinmeTokeni = std::make_shared<EventRegistrationToken>(); // Dinleyicinin kendi kendini kaldırabilmesi için, token'ın kendisi de paylaşılan bir yerde saklanır
                    onIsitmaWebView->add_NavigationCompleted(
                        Callback<ICoreWebView2NavigationCompletedEventHandler>(
                            [this, denetleyiciPaylasilan, onIsitmaWebView, yedekGezinmeTokeni](ICoreWebView2* gonderen, ICoreWebView2NavigationCompletedEventArgs* argumanlar) -> HRESULT
                            {
                                onIsitmaWebView->remove_NavigationCompleted(*yedekGezinmeTokeni); // Sadece ön ısıtma gezinmesi içindi, bir daha tetiklenmesin diye hemen kaldırılır
                                m_yedekHazirlananSayisi--; // Gezinme gerçekten bittiğine göre, artık "hazırlanmakta" sayılmaktan çıkar

                                YedekSekme yeniYedek; // Havuza eklenecek yeni yedek kaydı hazırlanır
                                yeniYedek.denetleyici = denetleyiciPaylasilan; // Bir sonraki yeni sekme isteğinde anında kullanılmak üzere saklanır
                                yeniYedek.hazirlananZaman = GetTickCount64(); // Bu yedeğin hazır hale geldiği an kaydedilir, uzun süre kullanılmazsa kapatılabilmesi için
                                m_yedekler.push_back(yeniYedek); // Yedek, gezinme GERÇEKTEN tamamlandıktan sonra havuza eklenir

                                YedekSekmeHazirla(); // Havuz hâlâ hedef boyutunun altındaysa, eksik kalan varsa tamamlanır
                                return S_OK; // İşlem başarıyla ele alındı
                            }
                        ).Get(),
                        yedekGezinmeTokeni.get()
                    );

                    onIsitmaWebView->Navigate(L"https://tarayici.local/newtab.html"); // En sık açılan dahili sayfa olduğu için, ön ısıtma hedefi olarak hızlı erişim sayfası seçilir
                    m_yedekAdresi = L"https://tarayici.local/newtab.html"; // Yedeğin hangi adrese önceden gezindirildiği kaydedilir, kullanım anında aynı adres istenirse tekrar gezinmekten kaçınılır

                    return S_OK; // İşlem başarıyla ele alındı, asıl havuza ekleme yukarıdaki NavigationCompleted dinleyicisinde gerçekleşecek
                }
            ).Get()
        );
    } // Havuz artık hedef boyutuna ulaştığına (veya ulaşacak kadar paralel istek başlatıldığına) göre, döngü burada sona erer
}

// Bu fonksiyon, arka planda uzun süredir kullanılmadan bekleyen yedekleri havuzdan kapatır
// Yedekler, kullanıcı sık sık yeni sekme açıyorsa değerlidir, ama kullanıcı uzun süre yeni sekme açmıyorsa, boşta duran bu denetleyicilerin tuttuğu süreçler tamamen gereksiz RAM harcar
// Bu fonksiyon, mevcut uyku zamanlayıcısı tarafından periyodik olarak çağrılır, böylece ayrı bir zamanlayıcıya gerek kalmaz
void BrowserWindow::YedekSekmeyiGerekirseKapat()
{
    if (m_yedekler.empty()) // Havuzda hiç yedek yoksa
    {
        return; // Yapılacak bir şey yoktur
    }

    const ULONGLONG yedekBosaZamanEsigiMs = 3ULL * 60ULL * 1000ULL; // Üç dakikadır kullanılmayan bir yedek, artık boşuna RAM tutuyor demektir
    ULONGLONG suankiZaman = GetTickCount64(); // Şu anki an alınır

    // Havuzdaki her yedek, kendi hazırlanma zamanına göre tek tek değerlendirilir, sadece yeterince uzun süredir boşta bekleyenler kapatılır
    // Havuz, YedekHavuzuHicUyumayanSayisi kadar yedeği HER ZAMAN diri tutar, bu yüzden havuz boyutu bu sayının altına düşecekse kapatma işlemi durdurulur
    for (size_t i = m_yedekler.size(); i > 0 && m_yedekler.size() > YedekHavuzuHicUyumayanSayisi; i--) // Listeden eleman silineceği için sondan başa doğru gezilir
    {
        size_t indeks = i - 1; // Geriye doğru gezinirken kullanılacak gerçek indeks
        if ((suankiZaman - m_yedekler[indeks].hazirlananZaman) > yedekBosaZamanEsigiMs) // Bu yedek yeterince uzun süredir boştaysa
        {
            m_yedekler[indeks].denetleyici->Close(); // Denetleyici kapatılır, bu da tuttuğu süreç ve belleği serbest bırakır, sadece referansı bırakmaktan daha güvenilir bir şekilde anında geri verir
            m_yedekler.erase(m_yedekler.begin() + indeks); // Yedek, havuzdan çıkarılır, döngünün başındaki koşul bir dahaki turda güncel boyutu zaten tekrar kontrol eder
            // Burada kasıtlı olarak YedekSekmeHazirla() TEKRAR çağrılmaz, kullanıcı gerçekten yeni bir sekme isteyene kadar (veya bir sonraki sekme açılışında) hiç RAM harcanmaz, bir dahaki ihtiyaçta zaten normal akış yeni yedekler hazırlayacaktır
        }
    }

    // Burada m_yedekler.empty() kontrolü kasıtlı olarak yapılmaz, çünkü YedekHavuzuHicUyumayanSayisi sıfırdan büyük olduğu sürece havuz asla tamamen boşalmaz
}

// Bu fonksiyon, verilen kimlik numarasına sahip sekmeyi sekme şeridinde seçili yapar, denetleyicisi zaten hazırsa ekrana da alır
void BrowserWindow::SekmeyeGec(int sekmeId)
{
    SekmeBilgisi* yeniAktifSekme = SekmeBul(sekmeId); // Aktif yapılmak istenen sekmenin bilgileri bulunur
    if (yeniAktifSekme == nullptr) // Eğer böyle bir sekme bulunamadıysa
    {
        return; // Yapılacak bir şey yoktur, fonksiyondan çıkılır
    }

    m_secimSekmeId = sekmeId; // Sekme şeridinde seçili görünmesi gereken sekme, denetleyicisi hazır olsun olmasın, her zaman hemen bu sekme olur
    DoldurmaPaneliniKapat(); // Sekme değişince, önceki sekmenin sayfasına göre konumlanmış otomatik doldurma önerisi artık geçersizdir

    if (yeniAktifSekme->denetleyici == nullptr) // Bu sekme henüz yeni açılmış ve denetleyicisi arka planda hazırlanıyor olabilir
    {
        // Ekranda o an GÖRÜNEN sekme (m_aktifSekmeId) bilerek değiştirilmez, kullanıcı asla boş veya çökmüş bir pencere görmez
        // Denetleyici hazır olduğunda, SekmeDenetleyicisiHazirlandi fonksiyonu m_secimSekmeId hâlâ bu sekmeyse gerçek görsel geçişi kendisi yapacaktır
        SekmeListesiniGonder(); // Şeritte en azından bu sekmenin artık seçili göründüğünden emin olunur
        return; // Gerçek içerik geçişi için, denetleyici hazır olmadan daha fazla bir şey yapılmaz
    }

    SekmeBilgisi* oncekiAktifSekme = AktifSekmeyiAl(); // Şu anda ekranda GÖRÜNEN (gerçek aktif) sekmenin bilgileri alınır
    if (oncekiAktifSekme != nullptr && oncekiAktifSekme->denetleyici != nullptr) // Eğer daha önce ekranda görünen ve denetleyicisi olan bir sekme varsa
    {
        oncekiAktifSekme->denetleyici->put_IsVisible(FALSE); // Bu sekme artık görünür olmaktan çıkarılır
    }

    if (yeniAktifSekme->askidaMi) // Eğer bu sekme uykuya alınmış durumdaysa
    {
        yeniAktifSekme->webViewGenisletilmis->Resume(); // Sekme, kullanıcı tekrar görmek istediği için uyandırılır
        yeniAktifSekme->askidaMi = false; // Artık uykuda olmadığı işaretlenir
    }

    yeniAktifSekme->sonAktifZaman = GetTickCount64(); // Bu sekmenin en son şimdi aktif olduğu kaydedilir, uyku zamanlayıcısı bu bilgiyi kullanır

    m_aktifSekmeId = sekmeId; // Bu sekme, artık aktif sekme olarak işaretlenir
    yeniAktifSekme->denetleyici->put_IsVisible(TRUE); // Bu sekme görünür yapılır
    BoyutlariGuncelle(); // Bu sekmenin, toolbar altındaki tüm alanı kaplaması için konumu güncellenir
    yeniAktifSekme->denetleyici->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC); // Yeni aktif sekmeye klavye odağı otomatik olarak verilir, kullanıcının önce tıklaması gerekmez

    GezinmeDurumunuGuncelleVeGonder(); // Bu sekmenin adres ve geri/ileri durumu toolbar arayüzüne gönderilir
    SekmeListesiniGonder(); // Hangi sekmenin aktif olduğu bilgisinin güncellenmesi için sekme listesi tekrar gönderilir
}

// Bu fonksiyon, verilen kimlik numarasına sahip sekmeyi kapatır
void BrowserWindow::SekmeKapat(int sekmeId)
{
    if (m_rizaOnayiBekleniyor && sekmeId == m_onboardingSekmeId) { return; } // Rıza onayı verilmeden karşılama sekmesi kapatılamaz

    bool kapatilanAktifti = (sekmeId == m_aktifSekmeId); // Kapatılan sekmenin, kapatılmadan önce ekranda GÖRÜNEN aktif sekme olup olmadığı not edilir
    bool kapatilanSeciliydi = (sekmeId == m_secimSekmeId); // Kapatılan sekmenin, denetleyicisi henüz hazır olmasa bile şeritte SEÇİLİ sekme olup olmadığı not edilir

    if (sekmeId == m_ayarlarSekmeId) // Eğer kapatılan sekme, Ayarlar sekmesiyse
    {
        m_ayarlarSekmeId = -1; // Artık açık bir Ayarlar sekmesi olmadığı işaretlenir, böylece bir dahaki açılışta yenisi oluşturulur
    }
    if (sekmeId == m_kisayollarSekmeId) // Eğer kapatılan sekme, kısayollar sekmesiyse
    {
        m_kisayollarSekmeId = -1; // Artık açık bir kısayollar sekmesi olmadığı işaretlenir
    }

    // Kapatılan sekmeye ait, henüz cevaplanmamış izin istekleri varsa, bunlar reddedilerek temizlenir, aksi halde ertelenmiş istek sonsuza kadar beklemede kalır ve bekleyenler listesinde kapanmış bir sekmeye ait geçersiz bir kayıt olarak asılı durur
    for (auto it = m_bekleyenIzinIstekleri.begin(); it != m_bekleyenIzinIstekleri.end(); ) // Bekleyen tüm izin istekleri sırayla kontrol edilir
    {
        if (it->second.sekmeId == sekmeId) // Eğer bu istek, kapatılan sekmeye aitse
        {
            it->second.argumanlar->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY); // İstek, sekme zaten kapandığı için reddedilmiş sayılır
            it->second.erteleme->Complete(); // Erteleme tamamlanır
            it = m_bekleyenIzinIstekleri.erase(it); // Kayıt, listeden çıkarılır, döngü bir sonraki geçerli öğeyle devam eder
        }
        else
        {
            ++it;
        }
    }
    IzinBildirimPanelindekiIstegiGuncelle(); // Gösterilmekte olan istek kapatılan sekmeye aitse panel gizlenir, değilse değişiklik olmaz

    // Kapatılan sekmeye ait, henüz cevaplanmamış sertifika hataları varsa, bunlar da (izin istekleriyle aynı sebepten) iptal edilerek temizlenir
    // Aksi halde ertelenmiş hata sonsuza kadar beklemede kalır ve bekleyenler listesinde kapanmış bir sekmeye ait geçersiz bir kayıt olarak asılı durur, bu da SONRAKİ gerçek sertifika hatalarının panelde hiç gösterilememesine sebep olur (panel her zaman en eski bekleyen kaydı gösterir)
    for (auto it = m_bekleyenSertifikaHatalari.begin(); it != m_bekleyenSertifikaHatalari.end(); ) // Bekleyen tüm sertifika hataları sırayla kontrol edilir
    {
        if (it->second.sekmeId == sekmeId) // Eğer bu hata, kapatılan sekmeye aitse
        {
            it->second.argumanlar->put_Action(COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL); // Hata, sekme zaten kapandığı için iptal edilmiş sayılır
            it->second.erteleme->Complete(); // Erteleme tamamlanır
            it = m_bekleyenSertifikaHatalari.erase(it); // Kayıt, listeden çıkarılır, döngü bir sonraki geçerli öğeyle devam eder
        }
        else
        {
            ++it;
        }
    }
    SertifikaPanelindekiHatayiGuncelle(); // Gösterilmekte olan hata kapatılan sekmeye aitse panel gizlenir, değilse değişiklik olmaz

    if (m_doldurmaSekmeId == sekmeId) // Otomatik doldurma paneli, şu anda kapatılan sekmeye hizmet ediyorsa
    {
        DoldurmaPaneliniKapat(); // Panel de kapatılır, kapanmış bir sekmeye artık şifre göndermeye çalışmaz
    }

    for (size_t i = 0; i < m_sekmeler.size(); i++) // Sekme listesindeki her sekme sırayla kontrol edilir
    {
        if (m_sekmeler[i]->id == sekmeId) // Eğer bu, kapatılmak istenen sekmeyse
        {
            // Kapatılan sekme gizli (özel) değilse ve en azından bir sayfa göstermeyi başarmışsa, "Son Kapatılan Sekmeyi Aç" için bir yığına kaydedilir
            // Gizli sekmeler burada da (tıpkı geçmişte olduğu gibi) bilerek hariç tutulur, çünkü bu yığın kalıcı değildir ama programın çalıştığı süre boyunca bir iz bırakır
            if (!m_sekmeler[i]->gizliMi && m_sekmeler[i]->webView != nullptr)
            {
                LPWSTR kapananAdresPtr = nullptr;
                m_sekmeler[i]->webView->get_Source(&kapananAdresPtr);
                std::wstring kapananAdres = kapananAdresPtr ? kapananAdresPtr : L"";
                if (kapananAdresPtr != nullptr) { CoTaskMemFree(kapananAdresPtr); }

                if (!kapananAdres.empty() && kapananAdres != L"https://tarayici.local/newtab.html") // Boş bir hızlı erişim sekmesini yeniden açmanın bir anlamı yoktur
                {
                    KapatilanSekmeBilgisi kayit;
                    kayit.adres = kapananAdres;
                    kayit.dahiliMi = m_sekmeler[i]->dahiliSayfaMi;
                    m_kapatilanSekmeler.push_back(kayit);
                    if (m_kapatilanSekmeler.size() > KapatilanSekmeYigitiHedefBoyutu) // Yığın, sınırsız büyümesin diye en eski kayıt atılır
                    {
                        m_kapatilanSekmeler.erase(m_kapatilanSekmeler.begin());
                    }
                }
            }

            if (m_sekmeler[i]->denetleyici != nullptr) // Bu sekmenin denetleyicisi henüz hiç hazırlanmamış olabilir, kullanıcı daha "Yükleniyor..." durumundayken sekmeyi kapatmış olabilir
            {
                m_sekmeler[i]->denetleyici->Close(); // Bu sekmenin WebView2 denetleyicisi ve kullandığı kaynaklar kapatılır
            }
            m_sekmeler.erase(m_sekmeler.begin() + i); // Sekme, listeden tamamen çıkarılır
            break; // Sekme bulunup kapatıldığına göre, döngüye devam etmeye gerek yoktur
        }
    }

    if (kapatilanAktifti) // Eğer kapatılan sekme, ekranda görünen aktif sekmeyse, yerine mutlaka başka bir sekmenin geçmesi gerekir
    {
        if (!m_sekmeler.empty()) // Eğer hâlâ açık başka sekmeler varsa
        {
            SekmeyeGec(m_sekmeler.back()->id); // Listedeki son sekmeye geçiş yapılır, bu çağrı m_secimSekmeId'yi de kendiliğinden günceller
        }
        else // Eğer hiç sekme kalmadıysa, yani kapatılan tek açık sekmeydi
        {
            m_aktifSekmeId = -1; // Aktif sekme olmadığı belirtilir
            m_secimSekmeId = -1; // Seçili sekme de olmadığı belirtilir
            DestroyWindow(m_pencere); // Son sekme kapatıldığında yerine yeni bir sekme açmak yerine, boş bir pencereyle kalmak istemeyen kullanıcılar için programın tamamı kapatılır
        }
    }
    else if (kapatilanSeciliydi) // Kapatılan sekme ekranda görünmüyordu ama şeritte seçiliydi (henüz denetleyicisi hazırlanıyordu), seçim ekranda hâlâ görünen sekmeye geri alınır
    {
        m_secimSekmeId = m_aktifSekmeId;
    }

    BosGruplariTemizle(); // Kapatılan sekme bir gruptaysa ve grupta başka sekme kalmadıysa, grup da listeden düşürülür
    SekmeListesiniGonder(); // Güncel sekme listesi, toolbar arayüzüne gönderilir
}

// Bu fonksiyon, verilen kimlik numaralı sekmenin sabitlenmiş (pinned) durumunu tersine çevirir
void BrowserWindow::SekmeyiSabitleKaldir(int sekmeId)
{
    SekmeBilgisi* sekme = SekmeBul(sekmeId); // Sabitlenecek/sabitliği kaldırılacak sekmenin bilgilerine erişilir
    if (sekme == nullptr) // Sekme bulunamazsa (örneğin bu arada kapatılmışsa)
    {
        return;
    }

    sekme->sabitliMi = !sekme->sabitliMi; // Durum tersine çevrilir

    // Sabitlenmiş sekmeler her zaman şeridin en solunda gösterilir, bu yüzden listedeki gerçek konumları da buna göre yeniden gruplanır
    // std::stable_partition, her grubun İÇİNDEKİ var olan sırayı bozmadan, sadece sabitli olanları öne alır
    std::stable_partition(m_sekmeler.begin(), m_sekmeler.end(), [](const std::unique_ptr<SekmeBilgisi>& s) { return s->sabitliMi; });

    SekmeListesiniGonder(); // Güncel sekme listesi (yeni sıra ve sabitlenme durumuyla) toolbar arayüzüne gönderilir
}

// Bu fonksiyon, verilen kimlik numaralı sekmenin güncel adresinde, aynı dahili/gizli durumuyla yeni bir kopya sekme açar
void BrowserWindow::SekmeyiCogalt(int sekmeId)
{
    SekmeBilgisi* sekme = SekmeBul(sekmeId); // Çoğaltılacak sekmenin bilgilerine erişilir
    if (sekme == nullptr || sekme->webView == nullptr) // Sekme bulunamazsa veya denetleyicisi henüz hazır değilse (hangi adreste olduğu bile bilinemez) çoğaltılamaz
    {
        return;
    }

    LPWSTR adresPtr = nullptr; // Çoğaltılacak sekmenin güncel adresini tutacak değişken
    sekme->webView->get_Source(&adresPtr);
    std::wstring adres = adresPtr ? adresPtr : L"";
    if (adresPtr != nullptr) { CoTaskMemFree(adresPtr); }

    if (adres.empty()) // Adres hiç okunamadıysa çoğaltmanın bir anlamı yoktur
    {
        return;
    }

    YeniSekmeOlustur(adres, true, sekme->dahiliSayfaMi, sekme->gizliMi); // Aynı adreste, aynı dahili/gizli durumuyla yeni bir sekme açılır ve hemen aktif yapılır
}

// Bu fonksiyon, verilen kimlik numaralı sekmenin sessize alınmış durumunu tersine çevirir
void BrowserWindow::SekmeSesiniDegistir(int sekmeId)
{
    SekmeBilgisi* sekme = SekmeBul(sekmeId);
    if (sekme == nullptr || sekme->webView == nullptr) { return; } // Sekme bulunamazsa veya denetleyicisi henüz hazır değilse yapılacak bir şey yoktur

    ComPtr<ICoreWebView2_8> webViewSurum8;
    sekme->webView.As(&webViewSurum8);
    if (webViewSurum8 == nullptr) { return; }

    sekme->sessizeAlindiMi = !sekme->sessizeAlindiMi; // Durum tersine çevrilir
    webViewSurum8->put_IsMuted(sekme->sessizeAlindiMi ? TRUE : FALSE); // WebView2'ye yeni durum bildirilir
    SekmeListesiniGonder(); // Sekme şeridindeki ses ikonunun güncellenmesi için liste tekrar gönderilir
}

// Grup renkleri için sabit, okunabilir bir palet, her yeni grup bu paletten sırayla bir renk alır
static const wchar_t* GrupRenkPaleti[] = { L"#ef4444", L"#f97316", L"#eab308", L"#22c55e", L"#3b82f6", L"#a855f7", L"#ec4899" };

// Bu fonksiyon, artık hiçbir sekmesi kalmayan grupları listeden temizler, grup oluşturma/çıkarma/kapatma işlemlerinden sonra çağrılır
void BrowserWindow::BosGruplariTemizle()
{
    std::vector<int> kullanimdakiGrupIdler;
    for (const std::unique_ptr<SekmeBilgisi>& sekme : m_sekmeler)
    {
        if (sekme->grupId != 0) { kullanimdakiGrupIdler.push_back(sekme->grupId); }
    }
    m_sekmeGruplari.erase(
        std::remove_if(m_sekmeGruplari.begin(), m_sekmeGruplari.end(), [&](const SekmeGrubu& grup)
        {
            return std::find(kullanimdakiGrupIdler.begin(), kullanimdakiGrupIdler.end(), grup.id) == kullanimdakiGrupIdler.end();
        }),
        m_sekmeGruplari.end()
    );
}

// Bu fonksiyon, reklam/izleyici engelleme ayarlarını diskten okuyup önbelleğe yazar
void BrowserWindow::EngellemeAyarlariniYenile()
{
    AyarVerisi ayarlar = Ayarlar::Oku();
    m_reklamEngellemeAcikMi = ayarlar.reklamEngellemeAcikMi;
    m_izleyiciEngellemeSeviyesi = ayarlar.izleyiciEngellemeSeviyesi;
}

// Bu fonksiyon, verilen sekmeyi içeren yeni bir grup oluşturur
void BrowserWindow::SekmeIcinYeniGrupOlustur(int sekmeId)
{
    SekmeBilgisi* sekme = SekmeBul(sekmeId);
    if (sekme == nullptr) { return; }

    SekmeGrubu yeniGrup;
    yeniGrup.id = m_sonrakiGrupId++;
    yeniGrup.isim = L"Grup " + std::to_wstring(yeniGrup.id);
    yeniGrup.renk = GrupRenkPaleti[(yeniGrup.id - 1) % (sizeof(GrupRenkPaleti) / sizeof(GrupRenkPaleti[0]))];
    m_sekmeGruplari.push_back(yeniGrup);

    int eskiGrupId = sekme->grupId;
    sekme->grupId = yeniGrup.id;
    if (eskiGrupId != 0) { BosGruplariTemizle(); } // Sekme başka bir gruptaydıysa, o grup artık boş kalmış olabilir

    SekmeListesiniGonder();
}

// Bu fonksiyon, verilen sekmeyi, verilen kimlikli mevcut bir gruba ekler
void BrowserWindow::SekmeyiGrubaEkle(int sekmeId, int grupId)
{
    SekmeBilgisi* sekme = SekmeBul(sekmeId);
    if (sekme == nullptr) { return; }

    bool grupVarMi = false;
    for (const SekmeGrubu& grup : m_sekmeGruplari) { if (grup.id == grupId) { grupVarMi = true; break; } }
    if (!grupVarMi) { return; } // Var olmayan bir gruba eklenemez

    int eskiGrupId = sekme->grupId;
    sekme->grupId = grupId;
    if (eskiGrupId != 0 && eskiGrupId != grupId) { BosGruplariTemizle(); }

    SekmeListesiniGonder();
}

// Bu fonksiyon, verilen sekmeyi bulunduğu gruptan çıkarır
void BrowserWindow::SekmeyiGruptanCikar(int sekmeId)
{
    SekmeBilgisi* sekme = SekmeBul(sekmeId);
    if (sekme == nullptr || sekme->grupId == 0) { return; }

    sekme->grupId = 0;
    BosGruplariTemizle();
    SekmeListesiniGonder();
}

// Bu fonksiyon, verilen kimlikli grubun adını değiştirir
void BrowserWindow::SekmeGrubunuAdlandir(int grupId, const std::wstring& yeniIsim)
{
    for (SekmeGrubu& grup : m_sekmeGruplari)
    {
        if (grup.id == grupId)
        {
            grup.isim = yeniIsim.empty() ? L"Grup" : yeniIsim;
            SekmeListesiniGonder();
            return;
        }
    }
}

// Bu fonksiyon, verilen kimlikli gruba ait tüm sekmeleri kapatır
void BrowserWindow::SekmeGrubunuKapat(int grupId)
{
    std::vector<int> kapatilacakSekmeIdler;
    for (const std::unique_ptr<SekmeBilgisi>& sekme : m_sekmeler)
    {
        if (sekme->grupId == grupId) { kapatilacakSekmeIdler.push_back(sekme->id); }
    }
    for (int sekmeId : kapatilacakSekmeIdler) { SekmeKapat(sekmeId); } // SekmeKapat, gruptaki son sekme kapandığında BosGruplariTemizle'nin tekrar çağrılmasına gerek bırakmadan grubu da listeden düşürür (aşağıdaki not)
}

// Bu fonksiyon, aktif sekme için WebView2'nin kendi yazdırma/PDF kaydetme önizleme penceresini açar
void BrowserWindow::SayfayiYazdir()
{
    SekmeBilgisi* aktifSekme = AktifSekmeyiAl();
    if (aktifSekme == nullptr || aktifSekme->webView == nullptr)
    {
        return;
    }

    ComPtr<ICoreWebView2_16> webViewSurum16; // ShowPrintUI fonksiyonu bu arayüz sürümünde bulunduğu için
    aktifSekme->webView.As(&webViewSurum16);
    if (webViewSurum16 != nullptr)
    {
        webViewSurum16->ShowPrintUI(COREWEBVIEW2_PRINT_DIALOG_KIND_BROWSER); // Tarayıcının kendi yazdırma önizlemesi açılır, burada "Yazıcıya Gönder" veya "PDF Olarak Kaydet" seçilebilir
    }
}

// Bu fonksiyon, en son kapatılan sekmeyi yeniden açar
void BrowserWindow::SonKapatilanSekmeyiAc()
{
    if (m_kapatilanSekmeler.empty()) // Yeniden açılabilecek kapatılmış bir sekme yoksa
    {
        return;
    }

    KapatilanSekmeBilgisi kayit = m_kapatilanSekmeler.back(); // Yığının tepesindeki (en son kapatılan) kayıt alınır
    m_kapatilanSekmeler.pop_back(); // Kayıt, yığından çıkarılır, aynı sekme Ctrl+Shift+T ile tekrar tekrar açılamaz

    YeniSekmeOlustur(kayit.adres, true, kayit.dahiliMi); // Sekme, kapandığı anki adresinde, aynı dahili durumuyla yeniden açılır ve hemen aktif yapılır
}

// Bu fonksiyon, kullanıcının sekme şeridinde sürükleyip bıraktığı yeni sıralamayı uygular
void BrowserWindow::SekmeleriYenidenSirala(const std::vector<int>& yeniSiraDizisi)
{
    if (yeniSiraDizisi.size() != m_sekmeler.size()) // Gelen liste, mevcut sekme sayısıyla eşleşmiyorsa (örneğin bu arada bir sekme açılmış/kapanmışsa), bayat/güvenilmez bir istektir
    {
        return;
    }

    std::vector<int> mevcutIdlerSirali; // Mevcut sekmelerin kimlikleri, karşılaştırma için sıralanmış halde toplanır
    for (const std::unique_ptr<SekmeBilgisi>& sekme : m_sekmeler) { mevcutIdlerSirali.push_back(sekme->id); }
    std::vector<int> yeniIdlerSirali = yeniSiraDizisi;
    std::sort(mevcutIdlerSirali.begin(), mevcutIdlerSirali.end());
    std::sort(yeniIdlerSirali.begin(), yeniIdlerSirali.end());

    if (mevcutIdlerSirali != yeniIdlerSirali) // Gelen liste, gerçekten var olan sekmelerin bir permütasyonu değilse (eksik, fazla veya tekrarlı bir kimlik içeriyorsa), güvenilmez kabul edilip hiçbir şey yapılmaz
    {
        return;
    }

    std::vector<std::unique_ptr<SekmeBilgisi>> yeniListe; // Yeni sıraya göre yeniden dizilecek liste
    yeniListe.reserve(m_sekmeler.size());
    for (int id : yeniSiraDizisi) // Gelen yeni sıradaki her kimlik için
    {
        for (std::unique_ptr<SekmeBilgisi>& sekme : m_sekmeler) // Mevcut listede bu kimliğe sahip sekme aranır
        {
            if (sekme != nullptr && sekme->id == id)
            {
                yeniListe.push_back(std::move(sekme)); // Sekmenin sahipliği, yeni listeye taşınır
                break;
            }
        }
    }
    m_sekmeler = std::move(yeniListe); // Sekme listesi, yeni sırasıyla değiştirilir

    // Kullanıcı sürükleme sırasında bir sabitli sekmeyi yanlışlıkla sabitsiz bölgeye taşımış olsa bile, sabitli sekmeler her zaman en solda kalacak şekilde burada yeniden garanti edilir
    std::stable_partition(m_sekmeler.begin(), m_sekmeler.end(), [](const std::unique_ptr<SekmeBilgisi>& s) { return s->sabitliMi; });

    SekmeListesiniGonder(); // Güncel sekme listesi (yeni sırayla) toolbar arayüzüne gönderilir
}

// Bu fonksiyon, verilen kimlik numarasına sahip sekmenin bilgilerini listeden bulup işaretçisini döndürür
SekmeBilgisi* BrowserWindow::SekmeBul(int sekmeId)
{
    for (std::unique_ptr<SekmeBilgisi>& sekme : m_sekmeler) // Sekme listesindeki her sekme sırayla kontrol edilir
    {
        if (sekme->id == sekmeId) // Eğer bu, aranan kimlik numarasına sahip sekmeyse
        {
            return sekme.get(); // Bu sekmenin ham işaretçisi döndürülür
        }
    }
    return nullptr; // Böyle bir sekme bulunamadıysa boş işaretçi döndürülür
}

// Bu fonksiyon, şu anda aktif olan sekmenin bilgilerini döndürür
SekmeBilgisi* BrowserWindow::AktifSekmeyiAl()
{
    return SekmeBul(m_aktifSekmeId); // Aktif sekmenin kimlik numarasıyla, SekmeBul fonksiyonu kullanılarak arama yapılır
}

// Bu fonksiyon, verilen kimlik numaralı sekmenin WebView2'sine, sayfa geçişi, başlık değişimi, çökme ve indirme gibi olayları dinleyen işleyicileri bağlar
// Bu fonksiyon, WebView2'nin ham izin türü değerini, bizim kendi diskte sakladığımız ve arayüzde kullandığımız basit metin anahtarına çevirir
// Burada listelenmeyen izin türleri için boş metin döndürülür, bu durumda istek hiç ele alınmaz, WebView2'nin kendi varsayılan davranışına bırakılır
static std::wstring IzinTuruMetne(COREWEBVIEW2_PERMISSION_KIND tur)
{
    switch (tur)
    {
        case COREWEBVIEW2_PERMISSION_KIND_GEOLOCATION: return L"konum";
        case COREWEBVIEW2_PERMISSION_KIND_CAMERA: return L"kamera";
        case COREWEBVIEW2_PERMISSION_KIND_MICROPHONE: return L"mikrofon";
        case COREWEBVIEW2_PERMISSION_KIND_NOTIFICATIONS: return L"bildirim";
        case COREWEBVIEW2_PERMISSION_KIND_AUTOPLAY: return L"otomatikoynatma";
        default: return L""; // Pano okuma, dosya erişimi, MIDI gibi daha nadir/teknik izinler kasıtlı olarak ele alınmaz
    }
}

void BrowserWindow::SekmeOlaylariniBagla(int sekmeId)
{
    SekmeBilgisi* sekme = SekmeBul(sekmeId); // İşleyicileri bağlanacak sekmenin bilgilerine erişilir
    if (sekme == nullptr) // Eğer sekme bulunamadıysa
    {
        return; // Yapılacak bir şey yoktur
    }

    // Sayfa yükleme tamamlandığında (başarılı veya başarısız) bu fonksiyon çalışır
    sekme->webView->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this, sekmeId](ICoreWebView2* gonderen, ICoreWebView2NavigationCompletedEventArgs* argumanlar) -> HRESULT
            {
                SekmeBilgisi* guncelSekme = SekmeBul(sekmeId); // Olay anındaki güncel sekme bilgisine, kimlik numarasıyla tekrar erişilir, çünkü sekme listesi bu sırada değişmiş olabilir
                if (guncelSekme == nullptr) // Eğer bu sekme bu sırada kapatılmışsa
                {
                    return S_OK; // Yapılacak bir şey kalmamıştır
                }

                guncelSekme->yukleniyor = false; // Sayfa yüklemesi artık tamamlandığı için yükleniyor durumu kapatılır

                BOOL basariliMi = FALSE; // Yüklemenin başarılı olup olmadığını tutacak değişken
                argumanlar->get_IsSuccess(&basariliMi); // Yüklemenin başarılı olup olmadığı bilgisi okunur

                if (basariliMi) // Eğer sayfa başarıyla yüklendiyse
                {
                    if (!guncelSekme->gizliMi) // Gizli sekmelerde ziyaret edilen sayfalar kasıtlı olarak geçmişe hiç kaydedilmez
                    {
                        LPWSTR guncelAdres = nullptr; // Yüklenen sayfanın adresini tutacak değişken
                        guncelSekme->webView->get_Source(&guncelAdres); // Güncel adres okunur
                        Depolama::GecmiseEkle(guncelAdres, guncelSekme->baslik); // Bu ziyaret, geçmiş listesine kaydedilir (dahili tarayici.local sayfaları zaten Depolama tarafında otomatik olarak hariç tutulur)
                        if (guncelAdres != nullptr && wcsncmp(guncelAdres, L"https://tarayici.local", 22) != 0) // Dahili sayfalar hariç, gerçek sitelerin ikonu şimdiden ısıtılır
                        {
                            // Google'ın favicon servisinden bu alan adının ikonu, sayfanın kendi içinde gizlice istenir, böylece tarayıcının paylaşılan HTTP önbelleğine düşer ve hızlı erişim sayfası açıldığında beklemeden görünür
                            guncelSekme->webView->ExecuteScript(L"(function(){try{var i=new Image();i.src='https://www.google.com/s2/favicons?domain='+encodeURIComponent(location.hostname)+'&sz=64';}catch(e){}})();", nullptr);
                        }
                        CoTaskMemFree(guncelAdres); // Ayrılan bellek serbest bırakılır
                    }
                }
                else // Eğer sayfa yüklenemediyse
                {
                    COREWEBVIEW2_WEB_ERROR_STATUS hataDurumu; // WebView2'nin bildirdiği ham hata durumunu tutacak değişken
                    argumanlar->get_WebErrorStatus(&hataDurumu); // Hatanın ne türde olduğu okunur

                    if (hataDurumu != COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_ABORTED) // Kullanıcının kendisi sayfa yüklemesini iptal ettiği durumlar bir gerçek hata olarak sayılmaz
                    {
                        std::wstring kod = AgHatasiniKoduCevir(hataDurumu); // Ham hata durumu, bizim kendi kategori ve numara içeren koduna çevrilir
                        LPWSTR basarisizAdres = nullptr; // Yüklenemeyen sayfanın adresini tutacak değişken
                        guncelSekme->webView->get_Source(&basarisizAdres); // Başarısız olan adres okunur
                        HataSayfasiGoster(sekmeId, kod, basarisizAdres ? basarisizAdres : L""); // Kullanıcıya kodlu bir hata sayfası gösterilir ve hata günlüğe kaydedilir
                        if (basarisizAdres != nullptr) { CoTaskMemFree(basarisizAdres); } // Ayrılan bellek, varsa serbest bırakılır
                    }
                }

                if (sekmeId == m_aktifSekmeId) // Eğer bu olay, şu anda ekranda gösterilen sekmeden geldiyse
                {
                    GezinmeDurumunuGuncelleVeGonder(); // Adres çubuğu ve geri/ileri butonlarının güncellenmesi için toolbar arayüzüne bilgi gönderilir
                }
                SekmeListesiniGonder(); // Yükleniyor durumu değiştiği için sekme şeridinin de güncellenmesi sağlanır
                return S_OK; // İşlem başarıyla ele alındı
            }
        ).Get(),
        nullptr
    );

    // Sayfa yüklemesi başladığında, sekmenin yükleniyor durumunun işaretlenmesi için bu fonksiyon çalışır
    sekme->webView->add_NavigationStarting(
        Callback<ICoreWebView2NavigationStartingEventHandler>(
            [this, sekmeId](ICoreWebView2* gonderen, ICoreWebView2NavigationStartingEventArgs* argumanlar) -> HRESULT
            {
                LPWSTR hedefAdresPtr = nullptr; // Gidilmek istenen adresi tutacak değişken
                argumanlar->get_Uri(&hedefAdresPtr); // Gidilen adres okunur
                std::wstring hedefAdres = hedefAdresPtr ? hedefAdresPtr : L""; // Adres, güvenli bir metin değişkenine aktarılır
                if (hedefAdresPtr != nullptr) { CoTaskMemFree(hedefAdresPtr); } // Ayrılan bellek serbest bırakılır

                if (m_rizaOnayiBekleniyor && hedefAdres != L"https://tarayici.local/onboarding.html") // Rıza onayı verilmeden, karşılama sayfası dışında hiçbir adrese gidilemez
                {
                    argumanlar->put_Cancel(TRUE); // İstenen gezinme iptal edilir
                    if (gonderen != nullptr) { gonderen->Navigate(L"https://tarayici.local/onboarding.html"); } // Kullanıcı, karşılama ekranına geri yönlendirilir
                    return S_OK;
                }

                // HTTPS zorunlu kılma: düz "http://" ile başlayan adresler, güvenli "https://" sürümüne yükseltilir
                // "localhost" ve "127.0.0.1" bilinçli olarak hariç tutulur, çünkü yerel geliştirme sunucularının genelde geçerli bir SSL sertifikası olmaz, onları da yükseltmek yerel geliştirme yapan kullanıcıları kırardı
                bool duzHttpMi = hedefAdres.rfind(L"http://", 0) == 0; // Adresin "http://" ile başlayıp başlamadığı kontrol edilir
                bool yerelAdresMi = hedefAdres.rfind(L"http://localhost", 0) == 0 || hedefAdres.rfind(L"http://127.0.0.1", 0) == 0; // Yerel geliştirme adresi olup olmadığı kontrol edilir
                if (duzHttpMi && !yerelAdresMi) // Yükseltilmesi gereken bir adresse
                {
                    argumanlar->put_Cancel(TRUE); // Düz HTTP gezinmesi iptal edilir
                    std::wstring guvenliAdres = L"https://" + hedefAdres.substr(7); // "http://" öneki (yedi karakter) "https://" ile değiştirilir
                    if (gonderen != nullptr) { gonderen->Navigate(guvenliAdres.c_str()); } // Yerine güvenli sürüme gezinilir
                    return S_OK; // Bu gezinme burada sona erer, aşağıdaki normal işleme devam edilmez
                }

                bool dahiliAdresMi = (hedefAdres.rfind(L"https://tarayici.local/", 0) == 0); // Uyarı sayfasının kendisi de dahil, dahili adresler güvenli gezinme kontrolünden her zaman muaftır
                if (!dahiliAdresMi && Ayarlar::Oku().guvenliGezinmeAcikMi)
                {
                    SekmeBilgisi* kontrolSekmesi = SekmeBul(sekmeId);
                    bool istisnaliMi = (kontrolSekmesi != nullptr && kontrolSekmesi->guvenliGezinmeIstisnaAdresi == hedefAdres); // Kullanıcı bu TEK adrese daha önce "Yine de Git" demişse, tekrar sorulmaz
                    if (!istisnaliMi && Engelleyici::SupheliAdresMi(hedefAdres))
                    {
                        argumanlar->put_Cancel(TRUE); // Şüpheli adrese gezinme iptal edilir
                        std::wstring hedefKodlanmis; // Hedef adres, sorgu parametresi olarak taşınabilmesi için yüzde kodlamasıyla (percent-encoding) kodlanır
                        for (wchar_t karakter : hedefAdres)
                        {
                            bool guvenliKarakterMi = iswalnum(karakter) || karakter == L'-' || karakter == L'_' || karakter == L'.' || karakter == L'~';
                            if (guvenliKarakterMi) { hedefKodlanmis += karakter; }
                            else
                            {
                                wchar_t tampon[8];
                                swprintf_s(tampon, L"%%%02X", static_cast<unsigned int>(karakter) & 0xFF);
                                hedefKodlanmis += tampon;
                            }
                        }
                        std::wstring uyariAdresi = L"https://tarayici.local/guvenli-gezinme-uyarisi.html?hedef=" + hedefKodlanmis;
                        if (gonderen != nullptr) { gonderen->Navigate(uyariAdresi.c_str()); }
                        return S_OK; // Bu gezinme burada sona erer, aşağıdaki normal işleme devam edilmez
                    }
                }

                SekmeBilgisi* guncelSekme = SekmeBul(sekmeId); // Güncel sekme bilgisine tekrar erişilir
                if (guncelSekme != nullptr) // Sekme hâlâ açıksa
                {
                    guncelSekme->yukleniyor = true; // Sayfa yükleniyor olarak işaretlenir

                    // Bu sekmenin dahili mi yoksa normal bir internet sitesi mi gösterdiği, HER gezinmede güncellenir, sadece sekme ilk oluşturulduğunda değil
                    // Bunun önemi şudur: her sekme, dahili hızlı erişim sayfasıyla (tarayici.local) başlar, ama kullanıcı oradan gerçek bir siteye geçtiğinde bu sekme artık dahili sayılmamalı, aksi halde uykuya alma mekanizması bu sekmeyi asla aday olarak görmez ve uzun süre açık kalan ağır siteler hiç RAM geri vermez
                    guncelSekme->dahiliSayfaMi = dahiliAdresMi; // Sadece gerçekten tarayici.local ile başlayan adresler dahili sayılır

                    // Yeni bir gezinme başladığı için, önceki sayfaya ait sertifika bilgileri artık geçersizdir, yeni sayfanın CDP olayı gelene kadar panelin eski sertifikayı göstermemesi için temizlenir
                    guncelSekme->sertifikaKonu.clear();
                    guncelSekme->sertifikaYayinlayan.clear();
                    guncelSekme->sertifikaProtokol.clear();
                    guncelSekme->favicon.clear(); // Yeni sayfanın kendi FaviconChanged olayı gelene kadar, eski sayfanın favicon'u sekme şeridinde kalmasın diye temizlenir

                    SekmeListesiniGonder(); // Sekme şeridinde yükleniyor göstergesinin belirmesi için liste tekrar gönderilir
                }
                return S_OK; // İşlem başarıyla ele alındı
            }
        ).Get(),
        nullptr
    );

    // Sekmenin başlığı değiştiğinde (sayfa kendi başlığını ayarladığında) bu fonksiyon çalışır
    sekme->webView->add_DocumentTitleChanged(
        Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
            [this, sekmeId](ICoreWebView2* gonderen, IUnknown* argumanlar) -> HRESULT
            {
                SekmeBilgisi* guncelSekme = SekmeBul(sekmeId); // Güncel sekme bilgisine tekrar erişilir
                if (guncelSekme != nullptr) // Sekme hâlâ açıksa
                {
                    LPWSTR yeniBaslik = nullptr; // Sayfanın yeni başlığını tutacak değişken
                    guncelSekme->webView->get_DocumentTitle(&yeniBaslik); // Güncel başlık okunur
                    guncelSekme->baslik = yeniBaslik ? yeniBaslik : L"Yeni Sekme"; // Sekmenin başlığı güncellenir, başlık okunamazsa varsayılan metin kullanılır
                    if (yeniBaslik != nullptr) { CoTaskMemFree(yeniBaslik); } // Ayrılan bellek serbest bırakılır
                    SekmeListesiniGonder(); // Yeni başlığın sekme şeridinde görünmesi için liste tekrar gönderilir
                }
                return S_OK; // İşlem başarıyla ele alındı
            }
        ).Get(),
        nullptr
    );

    // Sayfanın favicon'u değiştiğinde (veya ilk kez geldiğinde) bu fonksiyon çalışır
    ComPtr<ICoreWebView2_15> webViewSurum15Favicon; // FaviconChanged olayı ve GetFaviconAsync fonksiyonu bu arayüz sürümünde bulunduğu için
    if (SUCCEEDED(sekme->webView.As(&webViewSurum15Favicon)))
    {
        webViewSurum15Favicon->add_FaviconChanged(
            Callback<ICoreWebView2FaviconChangedEventHandler>(
                [this, sekmeId, gecerlilikBayragi = m_pencereGecerliMi](ICoreWebView2* gonderen, IUnknown* argumanlar) -> HRESULT
                {
                    SekmeBilgisi* guncelSekme = SekmeBul(sekmeId); // Güncel sekme bilgisine tekrar erişilir
                    if (guncelSekme == nullptr || guncelSekme->webView == nullptr) { return S_OK; } // Sekme hâlâ açık değilse vazgeçilir

                    ComPtr<ICoreWebView2_15> webViewSurum15;
                    if (FAILED(guncelSekme->webView.As(&webViewSurum15))) { return S_OK; }

                    webViewSurum15->GetFavicon(
                        COREWEBVIEW2_FAVICON_IMAGE_FORMAT_PNG,
                        Callback<ICoreWebView2GetFaviconCompletedHandler>(
                            [this, sekmeId, gecerlilikBayragi](HRESULT sonuc, IStream* akis) -> HRESULT
                            {
                                if (!*gecerlilikBayragi) { return S_OK; } // Pencere bu sırada kapatılmaya başladıysa "this" işaretçisine dokunulmadan vazgeçilir
                                SekmeBilgisi* guncelSekme2 = SekmeBul(sekmeId);
                                if (guncelSekme2 == nullptr || FAILED(sonuc) || akis == nullptr) { return S_OK; } // Sekme kapandıysa veya site hiç favicon vermediyse sessizce vazgeçilir, mevcut varsayılan logo gösterilmeye devam eder

                                LARGE_INTEGER sifirKonum{};
                                akis->Seek(sifirKonum, STREAM_SEEK_SET, nullptr);
                                STATSTG istatistik{};
                                akis->Stat(&istatistik, STATFLAG_NONAME);
                                ULONG boyut = static_cast<ULONG>(istatistik.cbSize.QuadPart);
                                if (boyut == 0) { return S_OK; } // Boş akış, site henüz favicon vermedi demektir, varsayılan logo korunur

                                std::vector<uint8_t> hamVeri(boyut);
                                ULONG okunanBayt = 0;
                                akis->Read(hamVeri.data(), boyut, &okunanBayt);
                                hamVeri.resize(okunanBayt);

                                guncelSekme2->favicon = L"data:image/png;base64," + Base64Kodla(hamVeri); // Ham PNG baytları, doğrudan bir <img>/arka plan resmine atanabilecek hazır veri adresine çevrilir
                                SekmeListesiniGonder(); // Yeni favicon'un sekme şeridinde görünmesi için liste tekrar gönderilir
                                return S_OK;
                            }
                        ).Get()
                    );
                    return S_OK;
                }
            ).Get(),
            nullptr
        );
    }

    // Geri/ileri gidilebilirlik durumu değiştiğinde bu fonksiyon çalışır
    sekme->webView->add_HistoryChanged(
        Callback<ICoreWebView2HistoryChangedEventHandler>(
            [this, sekmeId](ICoreWebView2* gonderen, IUnknown* argumanlar) -> HRESULT
            {
                if (sekmeId == m_aktifSekmeId) // Eğer bu, şu anda ekranda gösterilen sekmeyse
                {
                    GezinmeDurumunuGuncelleVeGonder(); // Güncel geri/ileri durumu toolbar arayüzüne gönderilir
                }
                return S_OK; // İşlem başarıyla ele alındı
            }
        ).Get(),
        nullptr
    );

    // Bu sekmenin render süreci beklenmedik şekilde kapanırsa (çökerse) bu fonksiyon çalışır
    sekme->webView->add_ProcessFailed(
        Callback<ICoreWebView2ProcessFailedEventHandler>(
            [this, sekmeId](ICoreWebView2* gonderen, ICoreWebView2ProcessFailedEventArgs* argumanlar) -> HRESULT
            {
                HataSayfasiGoster(sekmeId, L"UI-2010", L""); // Bu sekmede, çökme durumunu anlatan kodlu bir hata sayfası gösterilir ve günlüğe kaydedilir
                return S_OK; // İşlem başarıyla ele alındı
            }
        ).Get(),
        nullptr
    );

    // Bu sekmedeki bir site, konum/kamera/mikrofon/bildirim/otomatik oynatma gibi bir izin istediğinde bu fonksiyon çalışır
    sekme->webView->add_PermissionRequested(
        Callback<ICoreWebView2PermissionRequestedEventHandler>(
            [this, sekmeId](ICoreWebView2* gonderen, ICoreWebView2PermissionRequestedEventArgs* argumanlar) -> HRESULT
            {
                COREWEBVIEW2_PERMISSION_KIND tur; // İstenen izin türü
                argumanlar->get_PermissionKind(&tur);
                std::wstring turMetni = IzinTuruMetne(tur); // Türün bizim kullandığımız basit metin karşılığı bulunur
                if (turMetni.empty()) // Bizim ilgilenmediğimiz, nadir/teknik bir izin türüyse
                {
                    return S_OK; // Hiç karışılmaz, WebView2'nin kendi varsayılan davranışı (genelde reddetme) geçerli olur
                }

                LPWSTR kaynakPtr = nullptr; // İsteği yapan sitenin kaynağını (origin) tutacak değişken
                argumanlar->get_Uri(&kaynakPtr);
                std::wstring kaynak = kaynakPtr ? kaynakPtr : L"";
                if (kaynakPtr != nullptr) { CoTaskMemFree(kaynakPtr); }

                bool kayitliKarar = false; // Daha önce kaydedilmiş bir karar varsa buraya okunur
                if (IzinKasasi::KararBul(kaynak, turMetni, kayitliKarar)) // Bu site ve izin türü için daha önce bir karar kaydedilmişse
                {
                    argumanlar->put_State(kayitliKarar ? COREWEBVIEW2_PERMISSION_STATE_ALLOW : COREWEBVIEW2_PERMISSION_STATE_DENY); // Kayıtlı karar doğrudan uygulanır, kullanıcıya tekrar sorulmaz
                    return S_OK;
                }

                // Daha önce bir karar yoksa, WebView2'den bu isteği beklemeye alması istenir, kullanıcı izin panelinden karar verene kadar site bekletilir
                ComPtr<ICoreWebView2Deferral> erteleme;
                argumanlar->GetDeferral(&erteleme);

                int istekId = m_sonrakiIzinIstegiId++; // Bu isteğe, diğer bekleyen isteklerden ayırt edilebilmesi için yeni bir kimlik numarası verilir
                BekleyenIzinIstegi istek;
                istek.argumanlar = argumanlar;
                istek.erteleme = erteleme;
                istek.sekmeId = sekmeId;
                m_bekleyenIzinIstekleri[istekId] = istek; // İstek, cevap bekleyenler listesine eklenir

                IzinBildirimPanelindekiIstegiGuncelle(); // Panel, bu yeni isteği (hemen gösterilecekse) veya mevcut sıradaki isteği göstermeye devam eder
                return S_OK; // İşlem başarıyla ele alındı, karar daha sonra (erteleme tamamlanınca) verilecek
            }
        ).Get(),
        nullptr
    );

    // Bu sekmedeki bir sitenin HTTPS sertifikası güvenilmez/hatalı bulunduğunda bu fonksiyon çalışır (örneğin süresi dolmuş, adı uyuşmayan veya iptal edilmiş bir sertifika)
    // add_ServerCertificateErrorDetected, ICoreWebView2'nin daha yeni bir sürümünde (ICoreWebView2_14) bulunduğu için, önce bu arayüze geçilmeye çalışılır
    ComPtr<ICoreWebView2_14> sekmeWebView14;
    if (SUCCEEDED(sekme->webView.As(&sekmeWebView14)))
    {
        sekmeWebView14->add_ServerCertificateErrorDetected(
            Callback<ICoreWebView2ServerCertificateErrorDetectedEventHandler>(
                [this, sekmeId](ICoreWebView2* gonderen, ICoreWebView2ServerCertificateErrorDetectedEventArgs* argumanlar) -> HRESULT
                {
                    // Daha önce bu site için "yine de git" dendiyse, WebView2 bu durumu zaten kendi kendine hatırlar ve bu olay bir daha hiç tetiklenmez
                    // Bu yüzden buraya her geldiğimizde, kullanıcıya SORULMADAN önce varsayılan (güvenli) davranış devreye girsin diye erteleme alınır
                    ComPtr<ICoreWebView2Deferral> erteleme;
                    argumanlar->GetDeferral(&erteleme);

                    int istekId = m_sonrakiSertifikaHatasiId++; // Bu hataya, diğer bekleyen hatalardan ayırt edilebilmesi için yeni bir kimlik numarası verilir
                    BekleyenSertifikaHatasi hata;
                    hata.argumanlar = argumanlar;
                    hata.erteleme = erteleme;
                    hata.sekmeId = sekmeId;
                    m_bekleyenSertifikaHatalari[istekId] = hata; // Hata, cevap bekleyenler listesine eklenir

                    SertifikaPanelindekiHatayiGuncelle(); // Panel, bu yeni hatayı (hemen gösterilecekse) veya mevcut sıradaki hatayı göstermeye devam eder
                    return S_OK; // İşlem başarıyla ele alındı, karar daha sonra (erteleme tamamlanınca) verilecek
                }
            ).Get(),
            nullptr
        );
    }

    // Bu sekmede, kullanıcı bir görsele sağ tıkladığında, WebView2'nin kendi standart bağlam menüsüne "Resimde Ters Arama" adında ekstra bir öğe eklenir
    ComPtr<ICoreWebView2_11> sekmeWebView11;
    if (SUCCEEDED(sekme->webView.As(&sekmeWebView11)))
    {
        sekmeWebView11->add_ContextMenuRequested(
            Callback<ICoreWebView2ContextMenuRequestedEventHandler>(
                [this](ICoreWebView2* gonderen, ICoreWebView2ContextMenuRequestedEventArgs* argumanlar) -> HRESULT
                {
                    ComPtr<ICoreWebView2ContextMenuTarget> hedef; // Sağ tıklanan öğenin ne olduğunu (resim, metin, bağlantı gibi) anlatan nesne
                    argumanlar->get_ContextMenuTarget(&hedef);
                    if (hedef == nullptr) { return S_OK; }

                    COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND tur;
                    hedef->get_Kind(&tur);
                    if (tur != COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_IMAGE) { return S_OK; } // Sadece bir görsele sağ tıklandıysa ekstra öğe eklenir, diğer durumlarda menüye hiç dokunulmaz
                    BOOL kaynakVarMi = FALSE;
                    hedef->get_HasSourceUri(&kaynakVarMi);
                    if (!kaynakVarMi) { return S_OK; }

                    LPWSTR kaynakPtr = nullptr;
                    hedef->get_SourceUri(&kaynakPtr);
                    std::wstring gorselAdresi = kaynakPtr ? kaynakPtr : L"";
                    if (kaynakPtr != nullptr) { CoTaskMemFree(kaynakPtr); }
                    if (gorselAdresi.empty()) { return S_OK; }

                    ComPtr<ICoreWebView2Environment9> ortamSurum9; // Özel bir bağlam menüsü öğesi oluşturmak, daha yeni bir ortam arayüzünde bulunur
                    m_webViewOrtami.As(&ortamSurum9);
                    if (ortamSurum9 == nullptr) { return S_OK; }

                    ComPtr<ICoreWebView2ContextMenuItem> yeniOge;
                    ortamSurum9->CreateContextMenuItem(L"Resimde Ters Arama", nullptr, COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_COMMAND, &yeniOge);
                    if (yeniOge == nullptr) { return S_OK; }

                    yeniOge->add_CustomItemSelected(
                        Callback<ICoreWebView2CustomItemSelectedEventHandler>(
                            [this, gorselAdresi](ICoreWebView2ContextMenuItem* gonderenOge, IUnknown* argumanlar) -> HRESULT
                            {
                                // Google'ın "resimle arama" adresine, resmin adresi bir parametre olarak eklenir, yeni (gizli olmayan, normal) bir sekmede açılır
                                std::wstring aramaAdresi = L"https://www.google.com/searchbyimage?image_url=" + MetniAdresIcinKodla(gorselAdresi);
                                YeniSekmeOlustur(aramaAdresi, true, false);
                                return S_OK;
                            }
                        ).Get(),
                        nullptr
                    );

                    ComPtr<ICoreWebView2ContextMenuItemCollection> ogeListesi; // Menünün gerçek öğe listesi
                    argumanlar->get_MenuItems(&ogeListesi);
                    if (ogeListesi != nullptr)
                    {
                        ogeListesi->InsertValueAtIndex(0, yeniOge.Get()); // Yeni öğe, menünün en üstüne eklenir
                    }
                    return S_OK;
                }
            ).Get(),
            nullptr
        );
    }

    // Bu sekme odaktayken basılan klavye kısayollarının çalışması için, bu sekmenin denetleyicisine de kısayol dinleyicisi bağlanır
    sekme->denetleyici->add_AcceleratorKeyPressed(
        Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
            [this](ICoreWebView2Controller* gonderen, ICoreWebView2AcceleratorKeyPressedEventArgs* argumanlar) -> HRESULT
            {
                COREWEBVIEW2_KEY_EVENT_KIND olayTuru; // Bu tuş olayının türünü tutacak değişken
                argumanlar->get_KeyEventKind(&olayTuru); // Olay türü okunur
                if (olayTuru != COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN) // Sadece tuşa ilk basıldığı an ile ilgileniyoruz
                {
                    return S_OK; // Başka bir şey yapmaya gerek yok
                }
                UINT32 tusKodu = 0; // Basılan tuşun kodunu tutacak değişken
                argumanlar->get_VirtualKey(&tusKodu); // Tuş kodu okunur
                if (KisayolTusuIsle(tusKodu)) // Eğer bu tuş, tarayıcıya özel bir kısayolsa ve işlendiyse
                {
                    argumanlar->put_Handled(TRUE); // Tuşun sayfaya gitmesi engellenir
                }
                return S_OK; // İşlem başarıyla ele alındı
            }
        ).Get(),
        nullptr
    );

    // Bu sekmede bir dosya indirmesi başladığında bu fonksiyon çalışır
    sekme->webViewGenisletilmis->add_DownloadStarting(
        Callback<ICoreWebView2DownloadStartingEventHandler>(
            [this](ICoreWebView2* gonderen, ICoreWebView2DownloadStartingEventArgs* argumanlar) -> HRESULT
            {
                IndirmeBasladi(argumanlar); // İndirmenin onaylanıp onaylanmayacağına bu fonksiyonda karar verilir
                return S_OK; // İşlem başarıyla ele alındı
            }
        ).Get(),
        nullptr
    );
}

// Bu fonksiyon, güncel ayarlara göre toolbar'ın gerçekten kapladığı toplam yüksekliği hesaplar
int BrowserWindow::ToplamToolbarYuksekligi()
{
    return ToolbarYuksekligi + (Ayarlar::Oku().yerImiCubuguGoster ? YerImiCubuguYuksekligi : 0); // Yer imi çubuğu açıksa, sabit toolbar yüksekliğinin altına onun payı da eklenir
}

// Bu fonksiyon, pencere boyutu her değiştiğinde, toolbar ve aktif sekmenin WebView2'lerinin kapsayacağı alanları yeniden hesaplar
void BrowserWindow::BoyutlariGuncelle()
{
    RECT istemciAlani; // Pencerenin iç (çerçeve hariç) alanını tutacak dikdörtgen yapısı
    GetClientRect(m_pencere, &istemciAlani); // Pencerenin güncel iç alanı bu yapıya okunur, WM_NCCALCSIZE sayesinde bu alan zaten sol/sağ/alttaki gerçek boyutlandırma çerçevesini dışarıda bırakmış halde gelir

    if (m_toolbarDenetleyici != nullptr) // Eğer toolbar denetleyicisi henüz oluşmuşsa
    {
        RECT toolbarAlani = { 0, 0, istemciAlani.right, ToplamToolbarYuksekligi() }; // Toolbar, her zaman pencerenin en üstünde, tam genişlikte ve SABİT yükseklikte bir alan kaplar, hamburger menüsü artık bunu hiç büyütmez
        m_toolbarDenetleyici->put_Bounds(toolbarAlani); // Toolbar WebView2'sinin kaplayacağı alan bu dikdörtgen olarak ayarlanır
    }

    // DevTools paneli açıksa, içerik alanının sağ kenarı bu panelin genişliği kadar içeri çekilir, panel menünün aksine içeriğin üzerine binmez, gerçekten yanına yerleşir
    int devToolsGenisligi = (m_devToolsAcikMi && m_devToolsDenetleyici != nullptr) ? min(static_cast<int>(DevToolsPaneliGenisligi), istemciAlani.right / 2) : 0; // Çok dar pencerelerde panel, içeriği tamamen yutmasın diye en fazla pencere genişliğinin yarısı kadar yer kaplar

    SekmeBilgisi* aktifSekme = AktifSekmeyiAl(); // Şu anda aktif olan sekmenin bilgileri alınır
    if (aktifSekme != nullptr && aktifSekme->denetleyici != nullptr) // Eğer bir aktif sekme varsa ve denetleyicisi hazırsa
    {
        RECT icerikAlani = { 0, ToplamToolbarYuksekligi(), istemciAlani.right - devToolsGenisligi, istemciAlani.bottom }; // İçerik, her zaman sabit toolbar yüksekliğinin altından başlar, hamburger menüsünün açık/kapalı olması bunu hiç etkilemez, ama DevTools paneli açıksa sağ kenar ona göre daralır
        aktifSekme->denetleyici->put_Bounds(icerikAlani); // Aktif sekmenin kaplayacağı alan bu dikdörtgen olarak ayarlanır
    }

    if (m_devToolsAcikMi && m_devToolsDenetleyici != nullptr) // DevTools paneli açıksa, pencerenin sağına, içerik alanının tam yanına yerleştirilir
    {
        RECT devToolsAlani = { istemciAlani.right - devToolsGenisligi, ToplamToolbarYuksekligi(), istemciAlani.right, istemciAlani.bottom };
        m_devToolsDenetleyici->put_Bounds(devToolsAlani);
    }

    // Menü paneli açıksa, pencere yeniden boyutlandığında onun da konumu güncel pencereye göre tekrar hesaplanır
    if (m_hamburgerMenusuAcikMi && m_menuDenetleyici != nullptr)
    {
        int genislik = min(static_cast<int>(MenuPaneliGenisligi), istemciAlani.right); // Panel, çok dar pencerelerde bile taşmaz
        int yukseklik = min(static_cast<int>(MenuPaneliYuksekligi), istemciAlani.bottom - ToplamToolbarYuksekligi()); // Panel, çok kısa pencerelerde bile taşmaz
        RECT menuAlani = { istemciAlani.right - genislik, ToplamToolbarYuksekligi(), istemciAlani.right, ToplamToolbarYuksekligi() + max(0, yukseklik) }; // Panel, toolbar'ın hemen altında, sağa yaslı konumlanır
        m_menuDenetleyici->put_Bounds(menuAlani);
        MenuPaneliKoseleriniYuvarlaYap(menuAlani); // Pencere her yeniden boyutlandığında panelin konumu da değiştiği için, yuvarlak köşe kırpması da yeni konuma göre tekrar uygulanır
    }

    // İzin bildirim paneli gösterilecek bir istek varsa, pencere yeniden boyutlandığında onun da konumu güncel pencereye göre tekrar hesaplanır
    // Panel, adres çubuğunun hemen altında, ortaya yakın bir konumda belirir, çünkü isteği yapan "site" kavramı adres çubuğuyla ilişkilendirilir
    if (!m_bekleyenIzinIstekleri.empty() && m_izinDenetleyici != nullptr)
    {
        int genislik = min(static_cast<int>(IzinPaneliGenisligi), istemciAlani.right); // Panel, çok dar pencerelerde bile taşmaz
        int yukseklik = min(static_cast<int>(IzinPaneliYuksekligi), istemciAlani.bottom - ToplamToolbarYuksekligi()); // Panel, çok kısa pencerelerde bile taşmaz
        int sol = max(0, (istemciAlani.right - genislik) / 2); // Panel, pencerenin yatayda ortasına yakın konumlanır
        RECT izinAlani = { sol, ToplamToolbarYuksekligi(), sol + genislik, ToplamToolbarYuksekligi() + max(0, yukseklik) };
        m_izinDenetleyici->put_Bounds(izinAlani);
    }

    // Site bilgisi paneli açıksa, pencere yeniden boyutlandığında onun da konumu güncel pencereye göre tekrar hesaplanır
    // Panel, adres çubuğunun hemen altında, sola yakın (kilit ikonunun bulunduğu tarafa) konumlanır
    if (m_sitePaneliAcikMi && m_sitePaneliDenetleyici != nullptr)
    {
        int genislik = min(static_cast<int>(SitePaneliGenisligi), istemciAlani.right); // Panel, çok dar pencerelerde bile taşmaz
        int yukseklik = min(static_cast<int>(SitePaneliYuksekligi), istemciAlani.bottom - ToplamToolbarYuksekligi()); // Panel, çok kısa pencerelerde bile taşmaz
        RECT siteAlani = { 0, ToplamToolbarYuksekligi(), max(0, genislik), ToplamToolbarYuksekligi() + max(0, yukseklik) }; // Panel, toolbar'ın hemen altında, sola yaslı konumlanır
        m_sitePaneliDenetleyici->put_Bounds(siteAlani);
    }

    // Sertifika hatası paneli gösterilecek bir hata varsa, pencere yeniden boyutlandığında onun da konumu güncel pencereye göre tekrar hesaplanır
    // Panel, izin bildirim paneliyle aynı sebepten (hata, adres çubuğuyla ilişkili bir "site" kavramına ait) ortaya yakın bir konumda belirir
    if (!m_bekleyenSertifikaHatalari.empty() && m_sertifikaDenetleyici != nullptr)
    {
        int genislik = min(static_cast<int>(SertifikaPaneliGenisligi), istemciAlani.right); // Panel, çok dar pencerelerde bile taşmaz
        int yukseklik = min(static_cast<int>(SertifikaPaneliYuksekligi), istemciAlani.bottom - ToplamToolbarYuksekligi()); // Panel, çok kısa pencerelerde bile taşmaz
        int sol = max(0, (istemciAlani.right - genislik) / 2); // Panel, pencerenin yatayda ortasına yakın konumlanır
        RECT sertifikaAlani = { sol, ToplamToolbarYuksekligi(), sol + genislik, ToplamToolbarYuksekligi() + max(0, yukseklik) };
        m_sertifikaDenetleyici->put_Bounds(sertifikaAlani);
    }
}

// Bu fonksiyon, toolbar arayüzünden gelen metin komutunu okuyup, komuta uygun işlemi yapar
void BrowserWindow::ToolbarMesajiIsle(const std::wstring& mesaj)
{
    SekmeBilgisi* aktifSekme = AktifSekmeyiAl(); // Çoğu komut aktif sekme üzerinde çalıştığı için, aktif sekme bilgisi baştan alınır

    if (mesaj == L"back" && aktifSekme != nullptr) // Eğer gelen komut geri gitme komutuysa
    {
        aktifSekme->webView->GoBack(); // Aktif sekme bir önceki sayfaya döner
    }
    else if (mesaj == L"forward" && aktifSekme != nullptr) // Eğer gelen komut ileri gitme komutuysa
    {
        aktifSekme->webView->GoForward(); // Aktif sekme bir sonraki sayfaya gider
    }
    else if (mesaj == L"reload" && aktifSekme != nullptr) // Eğer gelen komut sayfayı yenileme komutuysa
    {
        aktifSekme->webView->Reload(); // Aktif sekme güncel sayfayı yeniden yükler
    }
    else if (mesaj == L"home" && aktifSekme != nullptr) // Eğer gelen komut ana sayfaya gitme komutuysa
    {
        aktifSekme->webView->Navigate(Ayarlar::Oku().anaSayfa.c_str()); // Aktif sekme, kullanıcının ayarladığı ana sayfa adresine gider
    }
    else if (mesaj == L"newtab") // Eğer gelen komut yeni sekme açma komutuysa
    {
        YeniSekmeOlustur(L"https://tarayici.local/newtab.html", true, true); // Yeni bir sekme, hızlı erişim sayfasıyla açılır ve hemen aktif yapılır
    }
    else if (mesaj == L"newprivatetab") // Eğer gelen komut yeni bir gizli sekme açma komutuysa
    {
        GizliSekmeAc(); // Gizli bir sekme açılır
    }
    else if (mesaj == L"opensettings") // Eğer gelen komut Ayarlar sekmesini açma komutuysa
    {
        AyarlarSekmesiniAc(); // Ayarlar sekmesi açılır, zaten açıksa ona geçilir
    }
    else if (mesaj.rfind(L"opensettings|", 0) == 0) // Eğer gelen komut, Ayarlar sekmesini belirli bir kategoriyle açma komutuysa (hamburger menüsündeki Geçmiş/Şifreler/Hata Günlüğü gibi kısayollar)
    {
        AyarlarSekmesiniAc(mesaj.substr(13)); // "opensettings|" önekinden sonraki kategori adıyla Ayarlar sekmesi açılır
    }
    else if (mesaj == L"openshortcuts") // Eğer gelen komut klavye kısayolları sekmesini açma komutuysa
    {
        KisayollarSekmesiniAc(); // Kısayollar sekmesi açılır, zaten açıksa ona geçilir
    }
    else if (mesaj == L"findinpage") // Eğer gelen komut, aktif sekme içinde sayfa içi arama çubuğunu açma komutuysa
    {
        SayfaIciAramaCubuguAcKapat(); // Arama çubuğu açılır/kapanır
    }
    else if (mesaj == L"zoomin" && aktifSekme != nullptr) // Eğer gelen komut yakınlaştırma komutuysa
    {
        ZoomDegistir(aktifSekme, 0.1); // Aktif sekme yakınlaştırılır
        GezinmeDurumunuGuncelleVeGonder(); // Güncel yakınlaştırma oranının menüde görünmesi için durum tekrar gönderilir
    }
    else if (mesaj == L"zoomout" && aktifSekme != nullptr) // Eğer gelen komut uzaklaştırma komutuysa
    {
        ZoomDegistir(aktifSekme, -0.1); // Aktif sekme uzaklaştırılır
        GezinmeDurumunuGuncelleVeGonder(); // Güncel yakınlaştırma oranının menüde görünmesi için durum tekrar gönderilir
    }
    else if (mesaj == L"zoomreset" && aktifSekme != nullptr) // Eğer gelen komut yakınlaştırmayı sıfırlama komutuysa
    {
        aktifSekme->denetleyici->put_ZoomFactor(1.0); // Aktif sekmenin yakınlaştırma oranı varsayılana döndürülür
        GezinmeDurumunuGuncelleVeGonder(); // Güncel yakınlaştırma oranının menüde görünmesi için durum tekrar gönderilir
    }
    else if (mesaj == L"get_ramstatus") // Eğer gelen komut, hamburger menüsündeki RAM göstergesi için güncel RAM kullanımını istiyorsa
    {
        DWORD ramMB = ToplamRamKullanimiMB(); // Tüm ilgili süreçlerin topladığı RAM, megabayt cinsinden hesaplanır
        MenuyeMesajGonder(L"{\"type\":\"ramstatus\",\"ramMB\":" + std::to_wstring(ramMB) + L"}"); // Hesaplanan değer menü paneline gönderilir
    }
    else if (mesaj == L"togglemenu") // Eğer gelen komut hamburger butonuna tıklandığını bildiriyorsa
    {
        if (m_hamburgerMenusuAcikMi) // Menü şu an açıksa
        {
            MenuyuKapat(); // Kapatılır
        }
        // Menü kapalıysa, SADECE bu kapanış az önce (hamburger butonuna tıklamanın tetiklediği LostFocus yüzünden) olmadıysa açılır
        // Aksi halde, butona basılı tutulan menüyü kapatmak isteyen ikinci tıklama, LostFocus'un kapattığı menüyü hemen tekrar açardı
        else if (GetTickCount64() - m_menuSonKapanmaZamani > 250)
        {
            MenuyuAc();
        }
    }
    else if (mesaj == L"closemenu") // Eğer gelen komut menünün kapatılması gerektiğini bildiriyorsa (bir öğeye tıklandı veya Escape'e basıldı)
    {
        MenuyuKapat();
    }
    else if (mesaj == L"togglesitepaneli") // Eğer gelen komut, adres çubuğundaki kilit ikonuna tıklandığını bildiriyorsa
    {
        if (m_sitePaneliAcikMi) // Panel şu an açıksa
        {
            SiteBilgisiPaneliniKapat(); // Kapatılır
        }
        // Panel kapalıysa, SADECE bu kapanış az önce (kilit ikonuna tıklamanın tetiklediği LostFocus yüzünden) olmadıysa açılır, hamburger menüsündekiyle aynı çakışma önleme mantığı
        else if (GetTickCount64() - m_sitePaneliSonKapanmaZamani > 250)
        {
            SiteBilgisiPaneliniAc();
        }
    }
    else if (mesaj == L"closesitepaneli") // Eğer gelen komut, site bilgisi panelinin kapatılması gerektiğini bildiriyorsa (Escape'e basıldı)
    {
        SiteBilgisiPaneliniKapat();
    }
    else if (mesaj.rfind(L"get_addresssuggestions|", 0) == 0) // Eğer gelen komut, adres çubuğuna yazılırken öneri istemiyse
    {
        AdresOnerileriniGosterVeGonder(mesaj.substr(23));
    }
    else if (mesaj == L"close_addresssuggestions") // Eğer gelen komut, öneri panelinin kapatılması gerektiğini bildiriyorsa
    {
        OneriPaneliniKapat();
    }
    else if (mesaj.rfind(L"addresssuggestion_choice|", 0) == 0 && aktifSekme != nullptr) // Eğer gelen komut, öneri panelindeki bir satıra tıklandığını bildiriyorsa
    {
        aktifSekme->webView->Navigate(mesaj.substr(25).c_str()); // Önerilen adres, zaten tam ve geçerli olduğu için doğrudan gidilir, adres çubuğunun akıllı yorumlamasından geçmesine gerek yoktur
        OneriPaneliniKapat();
    }
    else if (mesaj.rfind(L"addresssuggestion_navigate|", 0) == 0) // Eğer gelen komut, adres çubuğunda yukarı/aşağı ok tuşuna basıldığını bildiriyorsa
    {
        AdresOnerisiGezin(mesaj.substr(27));
    }
    else if (mesaj == L"addresssuggestion_confirm" && aktifSekme != nullptr) // Eğer gelen komut, Enter'a basıldığında klavyeyle seçili bir öneri olup olmadığının sorulmasıysa
    {
        if (!AdresOnerisiSeciliyiOnayla()) // Klavyeyle seçili bir satır yoksa, bu Enter basışı normal adres çubuğu gezinmesi olarak değerlendirilsin diye tarayıcı tarafına bildirilir
        {
            m_toolbarWebView->PostWebMessageAsString(L"{\"type\":\"addresssuggestion_notconfirmed\"}");
        }
    }
    else if (mesaj.rfind(L"site_paneli_izin_kaldir|", 0) == 0) // Eğer gelen komut, site bilgisi panelindeki bir izin kaydının kaldırılması isteğiyse
    {
        std::wstring tur = mesaj.substr(24); // "site_paneli_izin_kaldir|" önekinden sonraki izin türü alınır
        if (!m_sitePaneliKaynagi.empty()) // Panel gerçekten bir siteye ait olarak açıksa (güvenlik: hangi siteye uygulanacağı, panelden gelen veriye değil, native tarafın kendi hatırladığı kaynağa göre belirlenir)
        {
            IzinKasasi::KararSil(m_sitePaneliKaynagi, tur); // Kayıtlı karar silinir, bir sonraki istekte kullanıcıya tekrar sorulur
            SiteBilgisiPaneliVeriGonder(); // Panel, güncel (bir eksik) liste ile yeniden çizilir
        }
    }
    else if (mesaj.rfind(L"certificate_response|", 0) == 0) // Eğer gelen komut, sertifika hatası panelindeki "Geri Dön"/"Yine de Git" tıklamasıysa, biçimi "certificate_response|<istekId>|<proceed/cancel>" şeklindedir
    {
        std::wstring kalanMetin = mesaj.substr(21); // "certificate_response|" önekinden sonraki kısım alınır
        size_t ayracKonumu = kalanMetin.find(L'|');
        if (ayracKonumu != std::wstring::npos) // Komut doğru biçimdeyse
        {
            int istekId = _wtoi(kalanMetin.substr(0, ayracKonumu).c_str()); // İstek kimliği ayıklanır
            std::wstring karar = kalanMetin.substr(ayracKonumu + 1); // "proceed" veya "cancel" ayıklanır
            SertifikaHatasiYanitla(istekId, karar == L"proceed");
        }
    }
    else if (mesaj == L"get_theme") // Eğer toolbar arayüzü sayfası yeni yüklendiği için güncel temayı istiyorsa
    {
        TemayiToolbaraGonder(); // Kayıtlı tema ayarları toolbar arayüzüne gönderilir
    }
    else if (mesaj == L"get_tabs") // Eğer toolbar arayüzü sayfası yeni yüklendiği için güncel sekme listesini istiyorsa
    {
        // Program açılışında native taraf, toolbar sayfası yüklenmeden önce ilk sekmeyi oluşturup SekmeListesiniGonder çağırabiliyor, bu durumda o ilk mesaj kaybolur
        // Toolbar sayfası kendi yüklenmesi bittiğinde bu isteği gönderir, bu sayede güncel liste hiçbir zaman eksik kalmaz
        SekmeListesiniGonder(); // Güncel sekme listesi, aktif sekme bilgisiyle birlikte toolbar arayüzüne (yeniden) gönderilir
    }
    else if (mesaj == L"get_bookmarkbar") // Eğer toolbar arayüzü sayfası yeni yüklendiği için güncel yer imi çubuğu içeriğini istiyorsa
    {
        BookmarkBariGuncelleVeGonder(); // Kayıtlı yer imleri toolbar arayüzüne gönderilir
    }
    else if (mesaj.rfind(L"openbookmarkbar|", 0) == 0 && aktifSekme != nullptr) // Eğer gelen komut, yer imi çubuğundaki bir öğeye tıklandığını bildiriyorsa
    {
        aktifSekme->webView->Navigate(mesaj.substr(16).c_str()); // Aktif sekme, doğrudan bu kayıtlı adrese gider, adres çubuğundaki akıllı yorumlamadan (arama/adres ayrımı) geçmesine gerek yoktur çünkü bu adres zaten tam ve geçerlidir
    }
    else if (mesaj == L"addtoreadinglist" && aktifSekme != nullptr) // Eğer gelen komut, hamburger menüsünden aktif sayfayı okuma listesine ekleme komutuysa
    {
        LPWSTR guncelAdres = nullptr;
        aktifSekme->webView->get_Source(&guncelAdres);
        std::wstring adres = guncelAdres ? guncelAdres : L"";
        if (guncelAdres != nullptr) { CoTaskMemFree(guncelAdres); }
        if (!adres.empty()) { Depolama::OkumaListesineEkle(adres, aktifSekme->baslik); }
    }
    else if (mesaj == L"winminimize") // Pencereyi küçültme komutuysa
    {
        ShowWindow(m_pencere, SW_MINIMIZE); // Pencere görev çubuğuna küçültülür
    }
    else if (mesaj == L"winmaximize") // Pencereyi büyütme veya eski haline getirme komutuysa
    {
        if (IsZoomed(m_pencere)) // Eğer pencere şu anda büyütülmüş durumdaysa
        {
            ShowWindow(m_pencere, SW_RESTORE); // Pencere, büyütülmeden önceki boyutuna geri döndürülür
        }
        else // Eğer pencere şu anda normal boyuttaysa
        {
            ShowWindow(m_pencere, SW_MAXIMIZE); // Pencere ekranı kaplayacak şekilde büyütülür
        }
    }
    else if (mesaj == L"winclose") // Pencereyi kapatma komutuysa
    {
        PostMessage(m_pencere, WM_CLOSE, 0, 0); // Pencerenin normal kapanma sürecini başlatan standart mesaj gönderilir
    }
    else if (mesaj == L"windragstart") // Kullanıcı, toolbar arayüzündeki boş bir alana basılı tutup sürüklemeye başladıysa
    {
        // Pencere artık bir başlık çubuğu çizmediği için, sürükleyerek taşıma Windows'un kendiliğinden yapamayacağı bir şeydir
        // Bu iki satır, Windows'a "kullanıcı sanki başlık çubuğuna basmış gibi davran" diyerek, pencerenin fare ile sürüklenebilmesini sağlar, bu WebView2 uygulamalarında önerilen standart yöntemdir
        ReleaseCapture(); // Fare girdisinin WebView2 tarafından yakalanması bırakılır
        SendMessage(m_pencere, WM_NCLBUTTONDOWN, HTCAPTION, 0); // Pencereye, başlık çubuğuna tıklanmış gibi davranması söylenir, bu da Windows'un kendi sürükleme mekanizmasını devreye sokar
    }
    else if (mesaj.rfind(L"switchtab|", 0) == 0) // Eğer gelen komut bir sekmeye geçiş komutuysa
    {
        int sekmeId = _wtoi(mesaj.substr(10).c_str()); // "switchtab|" önekinden sonraki metin, sayıya çevrilir
        SekmeyeGec(sekmeId); // Belirtilen kimlik numaralı sekmeye geçiş yapılır
    }
    else if (mesaj.rfind(L"closetab|", 0) == 0) // Eğer gelen komut bir sekmeyi kapatma komutuysa
    {
        int sekmeId = _wtoi(mesaj.substr(9).c_str()); // "closetab|" önekinden sonraki metin, sayıya çevrilir
        SekmeKapat(sekmeId); // Belirtilen kimlik numaralı sekme kapatılır
    }
    else if (mesaj.rfind(L"gettabpreview|", 0) == 0) // Eğer gelen komut, fare bir sekmenin üzerinde beklerken o sekmenin küçük bir önizlemesini isteme komutuysa
    {
        int sekmeId = _wtoi(mesaj.substr(14).c_str()); // "gettabpreview|" önekinden sonraki metin, sayıya çevrilir
        SekmeOnizlemesiGonder(sekmeId); // Belirtilen sekmenin anlık görüntüsü yakalanıp toolbar arayüzüne gönderilir
    }
    else if (mesaj.rfind(L"pintab|", 0) == 0) // Eğer gelen komut bir sekmenin sabitlenme durumunu değiştirme komutuysa
    {
        int sekmeId = _wtoi(mesaj.substr(7).c_str()); // "pintab|" önekinden sonraki metin, sayıya çevrilir
        SekmeyiSabitleKaldir(sekmeId); // Belirtilen sekmenin sabitlenmiş durumu tersine çevrilir
    }
    else if (mesaj.rfind(L"duplicatetab|", 0) == 0) // Eğer gelen komut bir sekmeyi çoğaltma komutuysa
    {
        int sekmeId = _wtoi(mesaj.substr(13).c_str()); // "duplicatetab|" önekinden sonraki metin, sayıya çevrilir
        SekmeyiCogalt(sekmeId); // Belirtilen sekmenin bir kopyası açılır
    }
    else if (mesaj.rfind(L"mutetab|", 0) == 0) // Eğer gelen komut bir sekmenin sessize alınma durumunu değiştirme komutuysa
    {
        int sekmeId = _wtoi(mesaj.substr(8).c_str()); // "mutetab|" önekinden sonraki metin, sayıya çevrilir
        SekmeSesiniDegistir(sekmeId); // Belirtilen sekmenin sessize alınmış durumu tersine çevrilir
    }
    else if (mesaj.rfind(L"tabgroup_create|", 0) == 0) // Eğer gelen komut, bir sekme için yeni grup oluşturma komutuysa
    {
        SekmeIcinYeniGrupOlustur(_wtoi(mesaj.substr(16).c_str()));
    }
    else if (mesaj.rfind(L"tabgroup_add|", 0) == 0) // Eğer gelen komut, biçimi "tabgroup_add|<sekmeId>|<grupId>" olan, bir sekmeyi mevcut bir gruba ekleme komutuysa
    {
        std::wstring kalanMetin = mesaj.substr(13);
        size_t ayracKonumu = kalanMetin.find(L'|');
        if (ayracKonumu != std::wstring::npos)
        {
            int sekmeId = _wtoi(kalanMetin.substr(0, ayracKonumu).c_str());
            int grupId = _wtoi(kalanMetin.substr(ayracKonumu + 1).c_str());
            SekmeyiGrubaEkle(sekmeId, grupId);
        }
    }
    else if (mesaj.rfind(L"tabgroup_remove|", 0) == 0) // Eğer gelen komut, bir sekmeyi bulunduğu gruptan çıkarma komutuysa
    {
        SekmeyiGruptanCikar(_wtoi(mesaj.substr(16).c_str()));
    }
    else if (mesaj.rfind(L"tabgroup_rename|", 0) == 0) // Eğer gelen komut, biçimi "tabgroup_rename|<grupId>|<yeniIsim>" olan, bir grubu yeniden adlandırma komutuysa
    {
        std::wstring kalanMetin = mesaj.substr(16);
        size_t ayracKonumu = kalanMetin.find(L'|');
        if (ayracKonumu != std::wstring::npos)
        {
            int grupId = _wtoi(kalanMetin.substr(0, ayracKonumu).c_str());
            SekmeGrubunuAdlandir(grupId, kalanMetin.substr(ayracKonumu + 1));
        }
    }
    else if (mesaj.rfind(L"tabgroup_close|", 0) == 0) // Eğer gelen komut, bir grubu topluca kapatma komutuysa
    {
        SekmeGrubunuKapat(_wtoi(mesaj.substr(15).c_str()));
    }
    else if (mesaj == L"reopenclosedtab") // Eğer gelen komut, en son kapatılan sekmeyi yeniden açma komutuysa
    {
        SonKapatilanSekmeyiAc();
    }
    else if (mesaj == L"duplicateactivetab" && aktifSekme != nullptr) // Eğer gelen komut, hamburger menüsünden aktif sekmeyi çoğaltma komutuysa
    {
        SekmeyiCogalt(aktifSekme->id);
    }
    else if (mesaj == L"print") // Eğer gelen komut, aktif sayfayı yazdırma/PDF kaydetme komutuysa
    {
        SayfayiYazdir();
    }
    else if (mesaj == L"open_default_apps_settings") // Eğer gelen komut, Windows'un "Varsayılan Uygulamalar" ayar sayfasını açma komutuysa (varsayılan tarayıcı yapma istemi, karşılama ekranından veya Ayarlar'dan gelebilir)
    {
        ShellExecuteW(m_pencere, L"open", L"ms-settings:defaultapps", nullptr, nullptr, SW_SHOWNORMAL); // Kendi bir kurulum paketimiz (ve dolayısıyla Windows'a kayıtlı bir tarayıcı kimliğimiz) henüz olmadığı için, kullanıcıyı doğrudan kendi tarayıcımızı seçmeye zorlayamıyoruz, ama Windows'un kendi ilgili ayar sayfasını açarak bu seçimi kolaylaştırıyoruz
    }
    else if (mesaj == L"readeraloud" && aktifSekme != nullptr) // Eğer gelen komut, sayfayı sesli okuma komutuysa
    {
        SesliOkumayiBaslatDurdur();
    }
    else if (mesaj == L"readermode" && aktifSekme != nullptr) // Eğer gelen komut, okuyucu modunu açma komutuysa
    {
        OkuyucuModunuAc();
    }
    else if (mesaj == L"forcedarkmode" && aktifSekme != nullptr) // Eğer gelen komut, bu sitede zorunlu koyu modu açma/kapatma komutuysa
    {
        ZorunluKoyuModuAcKapat();
    }
    else if (mesaj == L"translatepage" && aktifSekme != nullptr && !aktifSekme->dahiliSayfaMi) // Eğer gelen komut, aktif sayfayı çevirme komutuysa, dahili sayfalar (Ayarlar gibi) çevrilmez
    {
        LPWSTR guncelAdresPtr = nullptr;
        aktifSekme->webView->get_Source(&guncelAdresPtr);
        std::wstring guncelAdres = guncelAdresPtr ? guncelAdresPtr : L"";
        if (guncelAdresPtr != nullptr) { CoTaskMemFree(guncelAdresPtr); }
        if (!guncelAdres.empty())
        {
            // Resmi bir çeviri API'si/anahtarı olmadan, Google Translate'in ücretsiz web proxy'si üzerinden sayfa yeniden yüklenir; bu yöntem birçok hafif tarayıcıda kullanılır ama resmi desteklenen bir API değildir, ileride çalışmayı durdurabilir
            std::wstring ceviriAdresi = L"https://translate.google.com/translate?sl=auto&tl=tr&u=" + guncelAdres;
            aktifSekme->webView->Navigate(ceviriAdresi.c_str());
        }
    }
    else if (mesaj.rfind(L"reordertabs|", 0) == 0) // Eğer gelen komut, sürükle-bırak ile sekmelerin yeniden sıralanması komutuysa, biçimi "reordertabs|<id1>,<id2>,<id3>..." şeklindedir
    {
        std::wstring idlerMetni = mesaj.substr(12); // "reordertabs|" önekinden sonraki, virgülle ayrılmış kimlikler listesi alınır
        std::vector<int> yeniSira; // Ayrıştırılan kimlikler, sırasıyla bu listeye eklenir
        size_t konum = 0;
        while (konum < idlerMetni.size()) // Metindeki her virgülle ayrılmış parça sırayla okunur
        {
            size_t virgulKonumu = idlerMetni.find(L',', konum);
            std::wstring parca = (virgulKonumu == std::wstring::npos) ? idlerMetni.substr(konum) : idlerMetni.substr(konum, virgulKonumu - konum);
            if (!parca.empty()) { yeniSira.push_back(_wtoi(parca.c_str())); }
            if (virgulKonumu == std::wstring::npos) { break; }
            konum = virgulKonumu + 1;
        }
        SekmeleriYenidenSirala(yeniSira); // Ayrıştırılan yeni sıra, kendi içinde doğrulanıp uygulanır
    }
    else if (mesaj.rfind(L"permission_response|", 0) == 0) // Eğer gelen komut, izin panelindeki "İzin Ver"/"Engelle" tıklamasıysa, biçimi "permission_response|<istekId>|<allow/deny>|<0/1>" şeklindedir
    {
        std::wstring kalanMetin = mesaj.substr(20); // "permission_response|" önekinden sonraki kısım alınır
        size_t birinciAyrac = kalanMetin.find(L'|');
        size_t ikinciAyrac = (birinciAyrac == std::wstring::npos) ? std::wstring::npos : kalanMetin.find(L'|', birinciAyrac + 1);
        if (birinciAyrac != std::wstring::npos && ikinciAyrac != std::wstring::npos) // Komut doğru biçimdeyse
        {
            int istekId = _wtoi(kalanMetin.substr(0, birinciAyrac).c_str()); // İstek kimliği ayıklanır
            std::wstring karar = kalanMetin.substr(birinciAyrac + 1, ikinciAyrac - birinciAyrac - 1); // "allow" veya "deny" ayıklanır
            std::wstring hatirlaMetni = kalanMetin.substr(ikinciAyrac + 1); // "0" veya "1" ayıklanır
            IzinIstegiYanitla(istekId, karar == L"allow", hatirlaMetni == L"1");
        }
    }
    else if (mesaj == L"star" && aktifSekme != nullptr) // Eğer gelen komut yer imi yıldızına tıklama komutuysa
    {
        LPWSTR guncelAdres = nullptr; // Aktif sekmenin güncel adresini tutacak değişken
        aktifSekme->webView->get_Source(&guncelAdres); // Güncel adres okunur
        std::wstring adres = guncelAdres ? guncelAdres : L""; // Adres, güvenli bir metin değişkenine aktarılır
        if (guncelAdres != nullptr) { CoTaskMemFree(guncelAdres); } // Ayrılan bellek serbest bırakılır

        if (Depolama::YerImindeMi(adres)) // Eğer bu sayfa zaten yer imlerindeyse
        {
            Depolama::YerImiKaldir(adres); // Yer iminden kaldırılır
        }
        else // Eğer bu sayfa henüz yer imlerinde değilse
        {
            Depolama::YerImiEkle(adres, aktifSekme->baslik); // Sayfa, güncel başlığıyla birlikte yer imlerine eklenir
        }
        GezinmeDurumunuGuncelleVeGonder(); // Yıldız butonunun yeni durumunun arayüze yansıması için güncel durum gönderilir
        BookmarkBariGuncelleVeGonder(); // Yer imi çubuğu, eklenen/kaldırılan yer imini anında yansıtacak şekilde güncellenir
    }
    else if (mesaj.rfind(L"navigate|", 0) == 0 && aktifSekme != nullptr) // Eğer gelen komut adres çubuğuna yazılan metni açma komutuysa
    {
        std::wstring girilenMetin = mesaj.substr(9); // "navigate|" önekinden sonraki asıl metin alınır
        AkilliAdreseGit(girilenMetin); // Bu metnin bir web adresi mi yoksa arama sorgusu mu olduğuna karar verilip ilgili adrese gidilir
    }
}

// Bu fonksiyon, dahili bir sayfadan (şu an için sadece Ayarlar sekmesinden) gelen mesajı okuyup, ayarlar/geçmiş/yer imi/şifre/günlük ile ilgili doğru işlemi yapar
void BrowserWindow::DahiliSayfaMesajiIsle(int sekmeId, const std::wstring& mesaj)
{
    if (mesaj == L"get_settings") // Güncel ayarlar isteniyorsa
    {
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Ayarlar JSON olarak gönderilir
    }
    else if (mesaj.rfind(L"safebrowsing_bypass|", 0) == 0) // Güvenli gezinme uyarısında kullanıcı "Yine de Git" dediyse
    {
        std::wstring hedefAdres = mesaj.substr(20); // "safebrowsing_bypass|" önekinden sonraki hedef adres alınır
        SekmeBilgisi* sekme = SekmeBul(sekmeId);
        if (sekme != nullptr && sekme->webView != nullptr && !hedefAdres.empty())
        {
            sekme->guvenliGezinmeIstisnaAdresi = hedefAdres; // Bu tek adrese bir dahaki NavigationStarting'de tekrar uyarı gösterilmez
            sekme->webView->Navigate(hedefAdres.c_str());
        }
    }
    else if (mesaj.rfind(L"set_searchengine|", 0) == 0) // Arama motoru değiştiriliyorsa
    {
        AyarVerisi ayarlar = Ayarlar::Oku(); // Mevcut ayarlar okunur
        ayarlar.aramaMotoru = mesaj.substr(17); // Yeni arama motoru atanır
        Ayarlar::Kaydet(ayarlar); // Ayarlar diske kaydedilir
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir, arayüzün değişikliği yansıtması için
    }
    else if (mesaj.rfind(L"set_homepage|", 0) == 0) // Ana sayfa değiştiriliyorsa
    {
        AyarVerisi ayarlar = Ayarlar::Oku(); // Mevcut ayarlar okunur
        ayarlar.anaSayfa = mesaj.substr(13); // Yeni ana sayfa atanır
        Ayarlar::Kaydet(ayarlar); // Ayarlar diske kaydedilir
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
    }
    else if (mesaj.rfind(L"set_lockminutes|", 0) == 0) // Şifre kasası kilitlenme süresi değiştiriliyorsa
    {
        AyarVerisi ayarlar = Ayarlar::Oku(); // Mevcut ayarlar okunur
        ayarlar.sifreKasasiKilitlenmeDakikasi = _wtoi(mesaj.substr(16).c_str()); // Yeni süre atanır
        Ayarlar::Kaydet(ayarlar); // Ayarlar diske kaydedilir
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
    }
    else if (mesaj.rfind(L"set_shortcuts|", 0) == 0) // Özelleştirilebilir klavye kısayolları değiştiriliyorsa
    {
        std::wstring veriJson = mesaj.substr(14); // "set_shortcuts|" önekinden sonraki küçük JSON nesnesi alınır
        AyarVerisi ayarlar = Ayarlar::Oku();

        auto harfOku = [&veriJson](const wchar_t* alanAdi, wchar_t varsayilan) -> wchar_t // Her alan için, gönderilen metnin ilk (ve tek) harfi okunur, boşsa önceki değeri korur
        {
            std::wstring okunan = JsonAlaniBul(veriJson, alanAdi);
            return okunan.empty() ? varsayilan : static_cast<wchar_t>(towupper(okunan[0])); // Küçük harf yazılmış olsa bile, karşılaştırmanın her zaman büyük harfle çalışması için büyük harfe çevrilir
        };
        ayarlar.kisayolYeniSekme = harfOku(L"newtab", ayarlar.kisayolYeniSekme);
        ayarlar.kisayolSekmeKapat = harfOku(L"closetab", ayarlar.kisayolSekmeKapat);
        ayarlar.kisayolCogalt = harfOku(L"duplicate", ayarlar.kisayolCogalt);
        ayarlar.kisayolYazdir = harfOku(L"print", ayarlar.kisayolYazdir);
        ayarlar.kisayolSayfadaBul = harfOku(L"find", ayarlar.kisayolSayfadaBul);
        ayarlar.kisayolYenile = harfOku(L"reload", ayarlar.kisayolYenile);
        ayarlar.kisayolAdres = harfOku(L"address", ayarlar.kisayolAdres);

        Ayarlar::Kaydet(ayarlar);
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
    }
    else if (mesaj.rfind(L"set_sleepthreshold|", 0) == 0) // Bellek tasarrufu modunun uykuya alma eşiği değiştiriliyorsa
    {
        AyarVerisi ayarlar = Ayarlar::Oku(); // Mevcut ayarlar okunur
        ayarlar.uykuSekmeEsigiDakika = _wtoi(mesaj.substr(19).c_str()); // Yeni eşik atanır
        Ayarlar::Kaydet(ayarlar); // Ayarlar diske kaydedilir
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
    }
    else if (mesaj.rfind(L"set_exitclean|", 0) == 0) // Çıkışta otomatik temizleme ayarı değiştiriliyorsa, biçim "set_exitclean|<evet/hayir>|<kategoriler>" şeklindedir
    {
        std::wstring kalanMetin = mesaj.substr(14); // "set_exitclean|" önekinden sonraki kısım alınır
        size_t ayracKonumu = kalanMetin.find(L'|');
        AyarVerisi ayarlar = Ayarlar::Oku();
        if (ayracKonumu != std::wstring::npos)
        {
            ayarlar.cikistaTemizle = (kalanMetin.substr(0, ayracKonumu) == L"evet");
            ayarlar.cikistaTemizleKategoriler = kalanMetin.substr(ayracKonumu + 1);
            Ayarlar::Kaydet(ayarlar);
        }
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
    }
    else if (mesaj.rfind(L"set_exemptsites|", 0) == 0) // Hariç tutulan site listesi güncelleniyorsa, biçim "set_exemptsites|<site1>|<site2>|..." şeklindedir, listenin tamamı her seferinde gönderilir
    {
        AyarVerisi ayarlar = Ayarlar::Oku();
        ayarlar.haricTutulanSiteler = mesaj.substr(16); // "set_exemptsites|" önekinden sonraki kısım, "|" ile ayrılmış liste olarak olduğu gibi saklanır
        Ayarlar::Kaydet(ayarlar);
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
    }
    else if (mesaj.rfind(L"set_askdownloadlocation|", 0) == 0) // "Her indirmede konum sor" ayarı değiştiriliyorsa
    {
        AyarVerisi ayarlar = Ayarlar::Oku();
        ayarlar.indirmeKonumunuHerSeferSor = (mesaj.substr(24) == L"evet");
        Ayarlar::Kaydet(ayarlar);
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
    }
    else if (mesaj.rfind(L"set_energysaver|", 0) == 0) // Enerji tasarrufu modu değiştiriliyorsa
    {
        AyarVerisi ayarlar = Ayarlar::Oku();
        ayarlar.enerjiTasarrufuModu = (mesaj.substr(16) == L"evet");
        Ayarlar::Kaydet(ayarlar);
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
        TemayiToolbaraGonder(); // Arayüz animasyonlarının kısılması için, güncel tema (energySaver dahil) toolbar'a da anında bildirilir
    }
    else if (mesaj.rfind(L"set_restoresession|", 0) == 0) // Önceki oturumu geri yükleme ayarı değiştiriliyorsa
    {
        AyarVerisi ayarlar = Ayarlar::Oku();
        ayarlar.sonOturumuGeriYukle = (mesaj.substr(19) == L"evet");
        Ayarlar::Kaydet(ayarlar);
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
        if (ayarlar.sonOturumuGeriYukle) { SekmeListesiniGonder(); } // Özellik şimdi açıldıysa, oturum dosyası hemen güncel sekmelerle yazılır, kullanıcı bir sekme daha açıp kapatmasına gerek kalmaz
    }
    else if (mesaj == L"pick_newtab_background") // Yeni sekme için özel bir arka plan resmi seçilmek isteniyorsa
    {
        wchar_t dosyaYoluTamponu[MAX_PATH] = {}; // Seçilen dosyanın tam yolu bu tampona yazılacak

        OPENFILENAMEW iletisimKutusu = {}; // Windows'un standart "Dosya Aç" penceresini açmak için gereken ayarlar
        iletisimKutusu.lStructSize = sizeof(OPENFILENAMEW);
        iletisimKutusu.hwndOwner = m_pencere;
        iletisimKutusu.lpstrFilter = L"Resim Dosyaları (*.jpg;*.jpeg;*.png;*.webp;*.gif)\0*.jpg;*.jpeg;*.png;*.webp;*.gif\0Tüm Dosyalar\0*.*\0";
        iletisimKutusu.lpstrFile = dosyaYoluTamponu;
        iletisimKutusu.nMaxFile = MAX_PATH;
        iletisimKutusu.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (GetOpenFileNameW(&iletisimKutusu)) // Kullanıcı bir dosya seçip onayladıysa (iptal ederse bu fonksiyon FALSE döner)
        {
            AyarVerisi ayarlar = Ayarlar::Oku();
            ayarlar.yeniSekmeArkaPlanYolu = dosyaYoluTamponu; // Seçilen dosyanın tam yolu kaydedilir, dosyanın kendisi kopyalanmaz, sadece yolu hatırlanır
            Ayarlar::Kaydet(ayarlar);
            AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar, Ayarlar sekmesine geri gönderilir
            YeniSekmeArkaPlaniniTumSekmelereGonder(); // Şu an açık olan hızlı erişim sekmeleri de yeni arka planı anında gösterir
        }
    }
    else if (mesaj == L"clear_newtab_background") // Özel arka plan resmi kaldırılmak isteniyorsa
    {
        AyarVerisi ayarlar = Ayarlar::Oku();
        ayarlar.yeniSekmeArkaPlanYolu.clear();
        Ayarlar::Kaydet(ayarlar);
        AyarlarIcinVeriGonder(sekmeId, L"settings");
        YeniSekmeArkaPlaniniTumSekmelereGonder();
    }
    else if (mesaj == L"get_newtab_background") // Hızlı erişim sayfası, özel arka plan resmini istiyorsa
    {
        YeniSekmeArkaPlaniniGonder(sekmeId);
    }
    else if (mesaj == L"get_backgroundgallery") // Ayarlar sayfası, "resimler" klasöründeki seçilebilir fotoğrafların listesini istiyorsa
    {
        ArkaPlanGalerisiniGonder(sekmeId);
    }
    else if (mesaj.rfind(L"set_newtab_background_frompath|", 0) == 0) // Kullanıcı, galerideki bir resme tıklayarak onu arka plan seçtiyse
    {
        KlasordenArkaPlanSec(sekmeId, mesaj.substr(32));
    }
    else if (mesaj.rfind(L"set_theme|", 0) == 0) // Tema (vurgu rengi, sekme yoğunluğu, yazı boyutu) değiştiriliyorsa
    {
        std::wstring veriJson = mesaj.substr(10); // "set_theme|" önekinden sonraki küçük JSON nesnesi alınır
        AyarVerisi ayarlar = Ayarlar::Oku(); // Mevcut ayarlar okunur

        std::wstring yeniVurguRengi = JsonAlaniBul(veriJson, L"accent"); // Gönderilen JSON'dan vurgu rengi okunur
        if (!yeniVurguRengi.empty()) { ayarlar.vurguRengi = yeniVurguRengi; } // Boş değilse ayarlara uygulanır

        std::wstring yeniYogunluk = JsonAlaniBul(veriJson, L"density"); // Gönderilen JSON'dan sekme yoğunluğu okunur
        if (!yeniYogunluk.empty()) { ayarlar.sekmeYogunlugu = yeniYogunluk; } // Boş değilse ayarlara uygulanır

        std::wstring yeniYaziBoyutu = JsonAlaniBul(veriJson, L"fontSize"); // Gönderilen JSON'dan yazı boyutu okunur
        if (!yeniYaziBoyutu.empty()) { ayarlar.yaziBoyutu = yeniYaziBoyutu; } // Boş değilse ayarlara uygulanır

        std::wstring yeniKose = JsonAlaniBul(veriJson, L"radius"); // Gönderilen JSON'dan köşe yuvarlaklığı okunur
        if (!yeniKose.empty()) { ayarlar.koseYuvarlakligi = yeniKose; } // Boş değilse ayarlara uygulanır

        std::wstring yeniMod = JsonAlaniBul(veriJson, L"mode"); // Gönderilen JSON'dan açık/koyu tema modu okunur
        if (!yeniMod.empty()) { ayarlar.temaModu = yeniMod; } // Boş değilse ayarlara uygulanır

        std::wstring yeniYerImiCubugu = JsonAlaniBul(veriJson, L"showBookmarkBar"); // Gönderilen JSON'dan yer imi çubuğu görünürlüğü okunur
        if (!yeniYerImiCubugu.empty()) { ayarlar.yerImiCubuguGoster = (yeniYerImiCubugu == L"evet"); } // Boş değilse ayarlara uygulanır

        Ayarlar::Kaydet(ayarlar); // Güncellenmiş ayarlar diske kaydedilir
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar, Ayarlar sekmesine geri gönderilir
        TemayiToolbaraGonder(); // Yeni tema, toolbar arayüzüne de anında yansıtılır
        BoyutlariGuncelle(); // Yer imi çubuğu açılıp/kapanmış olabileceği için, içerik alanının kapladığı alan yeniden hesaplanır
    }
    else if (mesaj.rfind(L"complete_onboarding|", 0) == 0) // Karşılama ekranı tamamlanıyorsa (kullanıcı seçimlerini yaptı veya atladı)
    {
        std::wstring veriJson = mesaj.substr(20); // "complete_onboarding|" önekinden sonraki küçük JSON nesnesi alınır
        AyarVerisi ayarlar = Ayarlar::Oku(); // Mevcut ayarlar okunur

        std::wstring secilenRenk = JsonAlaniBul(veriJson, L"accent"); // Kullanıcının seçtiği vurgu rengi okunur
        if (!secilenRenk.empty()) { ayarlar.vurguRengi = secilenRenk; } // Boş değilse ayarlara uygulanır

        std::wstring secilenMotor = JsonAlaniBul(veriJson, L"searchEngine"); // Kullanıcının seçtiği arama motoru okunur
        if (!secilenMotor.empty()) { ayarlar.aramaMotoru = secilenMotor; } // Boş değilse ayarlara uygulanır

        std::wstring rizaMetni = JsonAlaniBul(veriJson, L"consent"); // Kullanıcının rıza kutusunu işaretleyip işaretlemediği, diğer tüm bool alanlarla aynı biçimde "evet"/"hayir" metni olarak okunur
        bool rizaVerildiMi = (rizaMetni == L"evet"); // Arayüz zaten checkbox işaretlenmeden bu mesajı göndermiyor, ama native taraf burada ikinci bir güvence olarak kontrol eder

        if (rizaVerildiMi) // Rıza onayı verilmişse, program kullanılabilir hale getirilir; verilmemişse hiçbir şey değişmez ve engelleme sürer
        {
            ayarlar.ilkAcilisTamamlandiMi = true; // Karşılama ekranı artık tamamlandığı için, program bir daha asla göstermeyecek şekilde işaretlenir
            ayarlar.onaylananRizaSurumu = ProgramSurumu::Al(); // Hangi program sürümü için rıza verildiği kaydedilir, bir sonraki güncellemede sürüm değiştiğinde bu alan artık eşleşmeyeceği için onay ekranı yeniden gösterilir
            Ayarlar::Kaydet(ayarlar); // Güncellenmiş ayarlar diske kaydedilir

            m_rizaOnayiBekleniyor = false; // Engelleme kaldırılır: artık yeni sekme açılabilir ve her adrese gidilebilir

            TemayiToolbaraGonder(); // Seçilen vurgu rengi, toolbar arayüzüne anında yansıtılır

            SekmeBilgisi* sekme = SekmeBul(sekmeId); // Karşılama ekranını gösteren sekmenin bilgilerine erişilir
            if (sekme != nullptr) // Eğer sekme hâlâ açıksa
            {
                sekme->webView->Navigate(L"https://tarayici.local/newtab.html"); // Bu sekme, artık normal hızlı erişim sayfasına yönlendirilir
            }
        }
    }
    else if (mesaj == L"get_history") // Geçmiş listesi isteniyorsa
    {
        AyarlarIcinVeriGonder(sekmeId, L"history"); // Geçmiş JSON olarak gönderilir
    }
    else if (mesaj.rfind(L"history_delete_entry|", 0) == 0) // Geçmişten tek bir kaydın silinmesi isteniyorsa
    {
        std::wstring veriJson = mesaj.substr(21); // "history_delete_entry|" önekinden sonraki küçük JSON nesnesi alınır
        std::wstring adres = JsonAlaniBul(veriJson, L"adres"); // Silinecek kaydın adresi okunur
        std::wstring zaman = JsonAlaniBul(veriJson, L"zaman"); // Silinecek kaydın zaman damgası okunur
        Depolama::GecmistenKaydiSil(adres, zaman); // Adres ve zamanı eşleşen kayıt silinir
        AyarlarIcinVeriGonder(sekmeId, L"history"); // Güncel geçmiş listesi geri gönderilir
    }
    else if (mesaj.rfind(L"clear_data|", 0) == 0) // Geçmiş panelindeki ayrıntılı veri temizleme isteniyorsa, komutun biçimi "clear_data|<zamanKodu>|<kategoriler>" şeklindedir
    {
        std::wstring kalanMetin = mesaj.substr(11); // "clear_data|" önekinden sonraki kısım alınır
        size_t ayracKonumu = kalanMetin.find(L'|'); // Zaman kodu ile kategori listesini ayıran ikinci dikey çizgi aranır
        if (ayracKonumu != std::wstring::npos) // Komut doğru biçimdeyse
        {
            std::wstring zamanKodu = kalanMetin.substr(0, ayracKonumu); // Zaman kodu kısmı ayıklanır
            std::wstring kategorilerMetni = kalanMetin.substr(ayracKonumu + 1); // Kategori listesi kısmı ayıklanır
            VeriTemizle(sekmeId, zamanKodu, kategorilerMetni); // Asıl temizleme işlemi burada yapılır
        }
    }
    else if (mesaj == L"get_bookmarks") // Yer imi listesi isteniyorsa
    {
        AyarlarIcinVeriGonder(sekmeId, L"bookmarks"); // Yer imleri JSON olarak gönderilir
    }
    else if (mesaj.rfind(L"remove_bookmark|", 0) == 0) // Bir yer iminin kaldırılması isteniyorsa
    {
        Depolama::YerImiKaldir(mesaj.substr(16)); // Belirtilen adres, yer imlerinden kaldırılır
        AyarlarIcinVeriGonder(sekmeId, L"bookmarks"); // Güncel yer imi listesi geri gönderilir
        GezinmeDurumunuGuncelleVeGonder(); // Eğer kaldırılan sayfa aktif sekmedeyse, yıldız butonunun durumu da güncellensin diye
        BookmarkBariGuncelleVeGonder(); // Yer imi çubuğu güncellenir
    }
    else if (mesaj.rfind(L"move_bookmark|", 0) == 0) // Bir yer iminin başka bir klasöre taşınması isteniyorsa, biçimi "move_bookmark|<json>" şeklindedir, json {adres, klasor} içerir
    {
        std::wstring veriJson = mesaj.substr(14); // "move_bookmark|" önekinden sonraki küçük JSON nesnesi alınır
        std::wstring adres = JsonAlaniBul(veriJson, L"adres");
        std::wstring klasor = JsonAlaniBul(veriJson, L"klasor");
        Depolama::YerImiKlasoruDegistir(adres, klasor); // Belirtilen yer iminin klasörü güncellenir
        AyarlarIcinVeriGonder(sekmeId, L"bookmarks"); // Güncel yer imi listesi geri gönderilir
        BookmarkBariGuncelleVeGonder(); // Yer imi çubuğu güncellenir
    }
    else if (mesaj.rfind(L"reorder_bookmarks|", 0) == 0) // Yer imlerinin sürükle-bırak ile yeniden sıralanması isteniyorsa, biçimi "reorder_bookmarks|<adres1>|<adres2>|..." şeklindedir
    {
        std::wstring adreslerMetni = mesaj.substr(18); // "reorder_bookmarks|" önekinden sonraki, dikey çizgiyle ayrılmış adresler listesi alınır
        std::vector<std::wstring> yeniSira; // Ayrıştırılan adresler, sırasıyla bu listeye eklenir
        size_t konum = 0;
        while (konum < adreslerMetni.size()) // Metindeki her dikey çizgiyle ayrılmış parça sırayla okunur
        {
            size_t ayracKonumu = adreslerMetni.find(L'|', konum);
            std::wstring parca = (ayracKonumu == std::wstring::npos) ? adreslerMetni.substr(konum) : adreslerMetni.substr(konum, ayracKonumu - konum);
            if (!parca.empty()) { yeniSira.push_back(parca); }
            if (ayracKonumu == std::wstring::npos) { break; }
            konum = ayracKonumu + 1;
        }
        Depolama::YerImleriniSirala(yeniSira); // Ayrıştırılan yeni sıra, kendi içinde doğrulanıp uygulanır
        AyarlarIcinVeriGonder(sekmeId, L"bookmarks"); // Güncel (yeni sıralı) yer imi listesi geri gönderilir
        BookmarkBariGuncelleVeGonder(); // Yer imi çubuğu güncellenir
    }
    else if (mesaj == L"get_userscripts") // Kayıtlı kullanıcı betiklerinin listesi isteniyorsa
    {
        AyarlarIcinVeriGonder(sekmeId, L"userscripts");
    }
    else if (mesaj.rfind(L"add_userscript|", 0) == 0) // Yeni bir kullanıcı betiği ekleniyorsa
    {
        std::wstring veriJson = mesaj.substr(15);
        std::wstring ad = JsonAlaniBul(veriJson, L"name");
        std::wstring desen = JsonAlaniBul(veriJson, L"pattern");
        std::wstring kod = JsonAlaniBul(veriJson, L"code");
        BetikKasasi::BetikEkle(ad, desen, kod);
        AyarlarIcinVeriGonder(sekmeId, L"userscripts");
    }
    else if (mesaj.rfind(L"remove_userscript|", 0) == 0) // Bir kullanıcı betiğinin silinmesi isteniyorsa
    {
        BetikKasasi::BetikSil(mesaj.substr(18));
        AyarlarIcinVeriGonder(sekmeId, L"userscripts");
    }
    else if (mesaj.rfind(L"toggle_userscript|", 0) == 0) // Bir kullanıcı betiğinin etkinliğinin değiştirilmesi isteniyorsa, biçimi "toggle_userscript|<id>|<evet/hayir>" şeklindedir
    {
        std::wstring kalanMetin = mesaj.substr(18);
        size_t ayracKonumu = kalanMetin.find(L'|');
        if (ayracKonumu != std::wstring::npos)
        {
            std::wstring id = kalanMetin.substr(0, ayracKonumu);
            bool etkinMi = (kalanMetin.substr(ayracKonumu + 1) == L"evet");
            BetikKasasi::BetikEtkinligiDegistir(id, etkinMi);
        }
        AyarlarIcinVeriGonder(sekmeId, L"userscripts");
    }
    else if (mesaj == L"get_extensions") // Yüklü uzantıların listesi isteniyorsa
    {
        EklentileriniGonder(sekmeId);
    }
    else if (mesaj == L"pick_extension_folder") // Yeni bir uzantı klasörden yüklenmek isteniyorsa
    {
        EklentiKlasoruSecVeYukle(sekmeId);
    }
    else if (mesaj.rfind(L"load_extension_from_store|", 0) == 0) // Mağaza adresinden uzantı yüklenmek isteniyorsa
    {
        EklentiMagazaAdresindenYukle(sekmeId, mesaj.substr(27));
    }
    else if (mesaj == L"get_profiles") // Profil listesi isteniyorsa
    {
        ProfilListesiniGonder(sekmeId);
    }
    else if (mesaj.rfind(L"switch_profile|", 0) == 0) // Başka bir profile geçilmek isteniyorsa (boş isim, varsayılan/profilsiz moda geçiş demektir)
    {
        ProfileGecisYapVeYenidenBaslat(mesaj.substr(15));
    }
    else if (mesaj.rfind(L"create_profile|", 0) == 0) // Yeni bir profil oluşturulmak isteniyorsa, geçişten önce Windows şifresi istenir
    {
        if (Guvenlik::WindowsSifresiDogrula(m_pencere)) { ProfileGecisYapVeYenidenBaslat(mesaj.substr(15)); }
    }
    else if (mesaj.rfind(L"delete_profile|", 0) == 0) // Bir profilin kalıcı olarak silinmesi isteniyorsa, Windows şifresi istenir
    {
        ProfiliSil(sekmeId, mesaj.substr(15));
    }
    else if (mesaj.rfind(L"remove_extension|", 0) == 0) // Bir uzantının kaldırılması isteniyorsa
    {
        EklentiyiDegistir(sekmeId, mesaj.substr(17), L"remove");
    }
    else if (mesaj.rfind(L"enable_extension|", 0) == 0) // Bir uzantının etkinleştirilmesi isteniyorsa
    {
        EklentiyiDegistir(sekmeId, mesaj.substr(17), L"enable");
    }
    else if (mesaj.rfind(L"disable_extension|", 0) == 0) // Bir uzantının devre dışı bırakılması isteniyorsa
    {
        EklentiyiDegistir(sekmeId, mesaj.substr(18), L"disable");
    }
    else if (mesaj == L"get_readinglist") // Okuma listesi isteniyorsa
    {
        AyarlarIcinVeriGonder(sekmeId, L"readinglist"); // Okuma listesi JSON olarak gönderilir
    }
    else if (mesaj.rfind(L"remove_readinglist|", 0) == 0) // Okuma listesinden bir kaydın kaldırılması isteniyorsa
    {
        Depolama::OkumaListesindenKaldir(mesaj.substr(19)); // Belirtilen adres, okuma listesinden kaldırılır
        AyarlarIcinVeriGonder(sekmeId, L"readinglist"); // Güncel okuma listesi geri gönderilir
    }
    else if (mesaj == L"get_errorlog") // Hata günlüğü isteniyorsa
    {
        AyarlarIcinVeriGonder(sekmeId, L"errorlog"); // Son hatalar JSON olarak gönderilir
    }
    else if (mesaj == L"get_downloads") // İndirme listesi isteniyorsa
    {
        IndirmeListesiniGonder(); // Güncel liste, tüm dahili sekmelere (dolayısıyla isteyen İndirmeler paneline de) gönderilir
    }
    else if (mesaj.rfind(L"download_action|", 0) == 0) // Bir indirmenin duraklatılması, devam ettirilmesi veya iptal edilmesi isteniyorsa, biçimi "download_action|<id>|<pause/resume/cancel>" şeklindedir
    {
        std::wstring kalanMetin = mesaj.substr(16);
        size_t ayracKonumu = kalanMetin.find(L'|');
        if (ayracKonumu != std::wstring::npos)
        {
            int indirmeId = _wtoi(kalanMetin.substr(0, ayracKonumu).c_str());
            std::wstring eylem = kalanMetin.substr(ayracKonumu + 1);
            IndirmeEylemiUygula(indirmeId, eylem);
        }
    }
    else if (mesaj.rfind(L"download_showfolder|", 0) == 0) // Bir indirmenin klasörde gösterilmesi isteniyorsa
    {
        int indirmeId = _wtoi(mesaj.substr(20).c_str());
        IndirmeyiKlasordeGoster(indirmeId);
    }
    else if (mesaj == L"get_about") // Hakkında bilgisi (RAM kullanımı, çalışma süresi) isteniyorsa
    {
        AyarlarIcinVeriGonder(sekmeId, L"about"); // Hakkında bilgisi JSON olarak gönderilir
    }
    else if (mesaj == L"get_theme") // Dahili bir sayfa (örneğin hızlı erişim sayfası), güncel temayı kendi içinde uygulamak için istiyorsa
    {
        AyarVerisi a = Ayarlar::Oku(); // Güncel ayarlar diskten okunur
        std::wstring temaMesaji = L"{\"type\":\"theme\",\"theme\":" + TemaNesnesiJsonYap(a) + L"}"; // Tema bilgisi hazırlanır
        SekmeyeMesajGonder(sekmeId, temaMesaji); // Hazırlanan mesaj, isteyen sekmeye gönderilir
    }
    else if (mesaj == L"get_topsites") // Hızlı erişim sayfası, en çok ziyaret edilen siteleri istiyorsa
    {
        std::vector<SayfaKaydi> enCokZiyaretEdilenler = EnCokZiyaretEdilenler(8); // Geçmişten, en çok ziyaret edilen en fazla sekiz site hesaplanır
        std::wstring mesajMetni = L"{\"type\":\"topsites\",\"entries\":" + SayfaKayitlariniJsonYap(enCokZiyaretEdilenler) + L"}"; // Liste, bir JSON dizisi olarak sarmalanır
        SekmeyeMesajGonder(sekmeId, mesajMetni); // Hazırlanan mesaj, hızlı erişim sayfasına gönderilir
    }
    else if (mesaj == L"get_passwords") // Kayıtlı şifrelerin listesi isteniyorsa (bu liste şifrelerin kendisini içermez, sadece site ve kullanıcı adını içerir)
    {
        AyarlarIcinVeriGonder(sekmeId, L"passwords"); // Şifre listesi, şifreler gizli tutularak JSON olarak gönderilir
    }
    else if (mesaj == L"get_permissions") // Kaydedilmiş izin kararlarının listesi isteniyorsa
    {
        AyarlarIcinVeriGonder(sekmeId, L"permissions"); // İzin kararları JSON olarak gönderilir
    }
    else if (mesaj.rfind(L"remove_permission|", 0) == 0) // Kaydedilmiş bir izin kararının silinmesi isteniyorsa
    {
        std::wstring veriJson = mesaj.substr(18); // "remove_permission|" önekinden sonraki küçük JSON nesnesi alınır
        std::wstring kaynak = JsonAlaniBul(veriJson, L"kaynak"); // Kaynak (origin) okunur
        std::wstring tur = JsonAlaniBul(veriJson, L"tur"); // İzin türü okunur
        IzinKasasi::KararSil(kaynak, tur); // Belirtilen karar silinir, bir sonraki istekte kullanıcıya tekrar sorulur
        AyarlarIcinVeriGonder(sekmeId, L"permissions"); // Güncel izin listesi geri gönderilir
    }
    else if (mesaj.rfind(L"add_password|", 0) == 0) // Yeni bir şifre eklemesi isteniyorsa
    {
        std::wstring veriJson = mesaj.substr(13); // "add_password|" önekinden sonraki küçük JSON nesnesi alınır
        std::wstring site = JsonAlaniBul(veriJson, L"site"); // Site adresi okunur
        std::wstring kullaniciAdi = JsonAlaniBul(veriJson, L"username"); // Kullanıcı adı okunur
        std::wstring sifre = JsonAlaniBul(veriJson, L"password"); // Düz metin şifre okunur, bu sadece bu fonksiyonun içinde geçici olarak var olur
        if (!site.empty()) // Site boş değilse
        {
            SifreKasasi::SifreEkle(site, kullaniciAdi, sifre); // Şifre, DPAPI ile şifrelenerek kaydedilir
        }
        AyarlarIcinVeriGonder(sekmeId, L"passwords"); // Güncel şifre listesi geri gönderilir
    }
    else if (mesaj.rfind(L"remove_password|", 0) == 0) // Bir şifrenin silinmesi isteniyorsa
    {
        std::wstring veriJson = mesaj.substr(16); // "remove_password|" önekinden sonraki küçük JSON nesnesi alınır
        std::wstring site = JsonAlaniBul(veriJson, L"site"); // Site adresi okunur
        std::wstring kullaniciAdi = JsonAlaniBul(veriJson, L"username"); // Kullanıcı adı okunur
        SifreKasasi::SifreSil(site, kullaniciAdi); // Belirtilen kayıt silinir
        AyarlarIcinVeriGonder(sekmeId, L"passwords"); // Güncel şifre listesi geri gönderilir
    }
    else if (mesaj.rfind(L"reveal_password|", 0) == 0) // Kayıtlı bir şifrenin düz metin halinin gösterilmesi isteniyorsa
    {
        std::wstring veriJson = mesaj.substr(16); // "reveal_password|" önekinden sonraki küçük JSON nesnesi alınır
        std::wstring site = JsonAlaniBul(veriJson, L"site"); // Site adresi okunur
        std::wstring kullaniciAdi = JsonAlaniBul(veriJson, L"username"); // Kullanıcı adı okunur

        bool dogrulandi = Guvenlik::WindowsSifresiniKilitlenmeSuresiyleDogrula(m_pencere, Ayarlar::Oku().sifreKasasiKilitlenmeDakikasi); // Şifre gösterilmeden önce, kullanıcıdan Windows şifresi istenip doğrulanır, kısa süre önce doğrulandıysa (kullanıcının ayarladığı kilitlenme süresi dolmadıysa) tekrar sorulmaz

        std::wstring duzSifre; // Çözülecek düz metin şifreyi tutacak değişken, doğrulama başarısızsa boş kalır
        if (dogrulandi) // Eğer Windows şifresi doğru girildiyse
        {
            for (const SifreKaydi& kayit : SifreKasasi::SifreleriOku()) // Kayıtlı tüm şifreler gezilir
            {
                if (kayit.site == site && kayit.kullaniciAdi == kullaniciAdi) // Eğer bu, istenen kayıtsa
                {
                    duzSifre = SifreKasasi::SifreyiCoz(kayit.sifreliVeri); // Şifre, DPAPI ile çözülür
                    break; // Kayıt bulunduğuna göre döngüye devam etmeye gerek yoktur
                }
            }
        }

        // Sonuç, doğrulama başarısızsa şifre alanı "null" olacak şekilde, Ayarlar sekmesine geri gönderilir
        std::wstring cevap = L"{\"type\":\"password_revealed\",\"site\":\"" + JsonIcinKacisla(site) + L"\",\"username\":\"" + JsonIcinKacisla(kullaniciAdi) + L"\",\"password\":" + (dogrulandi ? (L"\"" + JsonIcinKacisla(duzSifre) + L"\"") : L"null") + L"}";
        SekmeyeMesajGonder(sekmeId, cevap); // Cevap, Ayarlar sekmesine gönderilir
    }
    else if (mesaj.rfind(L"set_language|", 0) == 0) // Site içeriği dili/yazım denetimi sözlüğü değiştiriliyorsa, etkili olması için program yeniden başlatılmalıdır
    {
        AyarVerisi ayarlar = Ayarlar::Oku();
        ayarlar.tarayiciDili = mesaj.substr(13);
        Ayarlar::Kaydet(ayarlar);
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
    }
    else if (mesaj.rfind(L"set_safebrowsing|", 0) == 0) // Güvenli gezinme uyarısı açık/kapalı ayarı değiştiriliyorsa
    {
        AyarVerisi ayarlar = Ayarlar::Oku();
        ayarlar.guvenliGezinmeAcikMi = (mesaj.substr(17) == L"evet");
        Ayarlar::Kaydet(ayarlar);
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
    }
    else if (mesaj.rfind(L"set_adblock|", 0) == 0) // Reklam engelleme açık/kapalı ayarı değiştiriliyorsa
    {
        AyarVerisi ayarlar = Ayarlar::Oku();
        ayarlar.reklamEngellemeAcikMi = (mesaj.substr(12) == L"evet");
        Ayarlar::Kaydet(ayarlar);
        EngellemeAyarlariniYenile(); // Önbellek hemen güncellenir, açık sekmelerdeki bir sonraki istekten itibaren yeni ayar geçerli olur
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
    }
    else if (mesaj.rfind(L"set_trackerlevel|", 0) == 0) // İzleyici engelleme seviyesi değiştiriliyorsa, biçim "set_trackerlevel|<kapali/temel/dengeli/siki>" şeklindedir
    {
        AyarVerisi ayarlar = Ayarlar::Oku();
        ayarlar.izleyiciEngellemeSeviyesi = mesaj.substr(17);
        Ayarlar::Kaydet(ayarlar);
        EngellemeAyarlariniYenile(); // Önbellek hemen güncellenir
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
    }
    else if (mesaj.rfind(L"set_breachcheck|", 0) == 0) // Sızıntı kontrolü özelliğinin açık/kapalı ayarı değiştiriliyorsa
    {
        AyarVerisi ayarlar = Ayarlar::Oku();
        ayarlar.sizintiKontroluAcikMi = (mesaj.substr(16) == L"evet");
        Ayarlar::Kaydet(ayarlar);
        AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar geri gönderilir
    }
    else if (mesaj.rfind(L"check_breach|", 0) == 0) // Kayıtlı bir şifrenin zayıflık ve sızıntı kontrolü isteniyorsa
    {
        std::wstring veriJson = mesaj.substr(13); // "check_breach|" önekinden sonraki küçük JSON nesnesi alınır
        std::wstring site = JsonAlaniBul(veriJson, L"site");
        std::wstring kullaniciAdi = JsonAlaniBul(veriJson, L"username");

        if (!Ayarlar::Oku().sizintiKontroluAcikMi) // Özellik ayarlardan kapatıldıysa (kullanıcı arada başka bir Ayarlar sekmesinden kapatmış olabilir), istek sessizce yok sayılır
        {
            return;
        }

        bool dogrulandi = Guvenlik::WindowsSifresiniKilitlenmeSuresiyleDogrula(m_pencere, Ayarlar::Oku().sifreKasasiKilitlenmeDakikasi); // Şifre çözülmeden önce, tıpkı "göster" işleminde olduğu gibi Windows şifresi doğrulanır

        std::wstring duzSifre;
        if (dogrulandi)
        {
            for (const SifreKaydi& kayit : SifreKasasi::SifreleriOku())
            {
                if (kayit.site == site && kayit.kullaniciAdi == kullaniciAdi) { duzSifre = SifreKasasi::SifreyiCoz(kayit.sifreliVeri); break; }
            }
        }

        if (!dogrulandi)
        {
            SizintiSonucu* sonuc = new SizintiSonucu();
            sonuc->sekmeId = sekmeId; sonuc->site = site; sonuc->kullaniciAdi = kullaniciAdi; sonuc->dogrulamaBasarisizMi = true;
            PostMessageW(m_pencere, SizintiSonucuMesaji, 0, reinterpret_cast<LPARAM>(sonuc));
            return;
        }

        bool zayifMi = SifreKasasi::SifreZayifMi(duzSifre); // Zayıflık kontrolü tamamen yerel olduğu için hemen yapılabilir
        std::wstring sha1Ozeti = SifreKasasi::SifreSha1OzetiAl(duzSifre);

        // İnternete istek atan kısım (sadece sızıntı kontrolü, zayıflık kontrolü değil), UI'yi bloklamaması için ayrı bir iş parçacığında yapılır
        HWND pencere = m_pencere;
        std::shared_ptr<bool> gecerlilikBayragi = m_pencereGecerliMi;
        std::thread([pencere, sekmeId, site, kullaniciAdi, zayifMi, sha1Ozeti, gecerlilikBayragi]()
        {
            bool sizdirilmisMi = false;
            int sizintiSayisi = 0;

            if (sha1Ozeti.size() == 40) // Geçerli bir SHA-1 özeti her zaman kırk onaltılık karakterdir
            {
                std::wstring onek = sha1Ozeti.substr(0, 5); // k-anonymity: sadece özetin ilk beş karakteri gönderilir, şifrenin kendisi veya özetin tamamı asla ağa çıkmaz
                std::wstring kalanHash = sha1Ozeti.substr(5);

                HINTERNET oturum = WinHttpOpen(L"Tarayici Sizinti Kontrolu", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
                if (oturum != nullptr)
                {
                    WinHttpSetTimeouts(oturum, 5000, 5000, 5000, 5000);
                    HINTERNET baglanti = WinHttpConnect(oturum, L"api.pwnedpasswords.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
                    if (baglanti != nullptr)
                    {
                        std::wstring yol = L"/range/" + onek;
                        HINTERNET istek = WinHttpOpenRequest(baglanti, L"GET", yol.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
                        if (istek != nullptr)
                        {
                            if (WinHttpSendRequest(istek, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(istek, nullptr))
                            {
                                std::string yanit;
                                DWORD mevcutBoyut = 0;
                                do
                                {
                                    mevcutBoyut = 0;
                                    if (!WinHttpQueryDataAvailable(istek, &mevcutBoyut) || mevcutBoyut == 0) { break; }
                                    std::vector<char> arabellek(mevcutBoyut);
                                    DWORD okunanBayt = 0;
                                    if (WinHttpReadData(istek, arabellek.data(), mevcutBoyut, &okunanBayt)) { yanit.append(arabellek.data(), okunanBayt); }
                                } while (mevcutBoyut > 0);

                                // Yanıt, her satırı "KALAN_HASH:SAYI" biçiminde satırlardan oluşur (CRLF ile ayrılmış), eşleşen satır aranır
                                std::wstring yanitGenis = Utf8iGenisMetneCevir(yanit);
                                size_t konum = 0;
                                while (konum < yanitGenis.size())
                                {
                                    size_t satirSonu = yanitGenis.find(L'\n', konum);
                                    std::wstring satir = (satirSonu == std::wstring::npos) ? yanitGenis.substr(konum) : yanitGenis.substr(konum, satirSonu - konum);
                                    while (!satir.empty() && (satir.back() == L'\r')) { satir.pop_back(); }

                                    size_t ayracKonumu = satir.find(L':');
                                    if (ayracKonumu != std::wstring::npos)
                                    {
                                        std::wstring satirHash = satir.substr(0, ayracKonumu);
                                        if (satirHash == kalanHash)
                                        {
                                            sizdirilmisMi = true;
                                            sizintiSayisi = _wtoi(satir.substr(ayracKonumu + 1).c_str());
                                            break;
                                        }
                                    }
                                    if (satirSonu == std::wstring::npos) { break; }
                                    konum = satirSonu + 1;
                                }
                            }
                            WinHttpCloseHandle(istek);
                        }
                        WinHttpCloseHandle(baglanti);
                    }
                    WinHttpCloseHandle(oturum);
                }
            }

            if (!*gecerlilikBayragi) { return; } // Pencere bu arka plan işlemi sürerken kapatılmaya başladıysa, mesaj göndermeye bile gerek yoktur

            SizintiSonucu* sonuc = new SizintiSonucu();
            sonuc->sekmeId = sekmeId; sonuc->site = site; sonuc->kullaniciAdi = kullaniciAdi; sonuc->zayifMi = zayifMi; sonuc->sizdirilmisMi = sizdirilmisMi; sonuc->sizintiSayisi = sizintiSayisi;
            PostMessageW(pencere, SizintiSonucuMesaji, 0, reinterpret_cast<LPARAM>(sonuc));
        }).detach();
    }
    else if (mesaj == L"get_autofillprofile") // Kişisel bilgi profili isteniyorsa
    {
        OtomatikDoldurmaProfili profil = OtomatikDoldurmaKasasi::ProfilOku(); // Profil diskten okunur
        std::wstring cevap = L"{\"type\":\"autofillprofile\",\"adSoyad\":\"" + JsonIcinKacisla(profil.adSoyad) + L"\",\"eposta\":\"" + JsonIcinKacisla(profil.eposta) + L"\",\"telefon\":\"" + JsonIcinKacisla(profil.telefon) + L"\",\"adres\":\"" + JsonIcinKacisla(profil.adres) + L"\"}";
        SekmeyeMesajGonder(sekmeId, cevap);
    }
    else if (mesaj.rfind(L"set_autofillprofile|", 0) == 0) // Kişisel bilgi profili kaydediliyorsa
    {
        std::wstring veriJson = mesaj.substr(20); // "set_autofillprofile|" önekinden sonraki küçük JSON nesnesi alınır
        OtomatikDoldurmaProfili profil;
        profil.adSoyad = JsonAlaniBul(veriJson, L"adSoyad");
        profil.eposta = JsonAlaniBul(veriJson, L"eposta");
        profil.telefon = JsonAlaniBul(veriJson, L"telefon");
        profil.adres = JsonAlaniBul(veriJson, L"adres");
        OtomatikDoldurmaKasasi::ProfilKaydet(profil);
    }
    else if (mesaj == L"get_cards") // Kayıtlı kartların listesi isteniyorsa (bu liste kart numaralarını içermez, sadece kart sahibi ve son kullanma tarihini içerir)
    {
        std::vector<KayitliKart> liste = OtomatikDoldurmaKasasi::KartlariOku(); // Kartlar diskten okunur
        std::wstring girdilerMetni = L"[";
        for (size_t i = 0; i < liste.size(); i++)
        {
            if (i > 0) { girdilerMetni += L","; }
            // Dikkat: kartNumarasiSifreli alanı kasıtlı olarak bu mesaja hiç dahil edilmez, arayüz kart numarasını sadece reveal_card isteğiyle ve Windows doğrulamasından sonra görebilir
            girdilerMetni += L"{\"kartSahibi\":\"" + JsonIcinKacisla(liste[i].kartSahibi) + L"\",\"sonKullanma\":\"" + JsonIcinKacisla(liste[i].sonKullanma) + L"\",\"eklenmeZamani\":\"" + JsonIcinKacisla(liste[i].eklenmeZamani) + L"\"}";
        }
        girdilerMetni += L"]";
        SekmeyeMesajGonder(sekmeId, L"{\"type\":\"cards\",\"entries\":" + girdilerMetni + L"}");
    }
    else if (mesaj.rfind(L"add_card|", 0) == 0) // Yeni bir kart ekleniyorsa
    {
        std::wstring veriJson = mesaj.substr(9); // "add_card|" önekinden sonraki küçük JSON nesnesi alınır
        std::wstring kartSahibi = JsonAlaniBul(veriJson, L"holder");
        std::wstring numara = JsonAlaniBul(veriJson, L"number");
        std::wstring sonKullanma = JsonAlaniBul(veriJson, L"expiry");
        OtomatikDoldurmaKasasi::KartEkle(kartSahibi, numara, sonKullanma); // Kart numarası, burada şifrelenip kaydedilir, düz metin hiçbir zaman diske yazılmaz
        AyarlarIcinVeriGonder(sekmeId, L"cards"); // Güncel kart listesi geri gönderilir
    }
    else if (mesaj.rfind(L"remove_card|", 0) == 0) // Bir kartın silinmesi isteniyorsa
    {
        std::wstring veriJson = mesaj.substr(12); // "remove_card|" önekinden sonraki küçük JSON nesnesi alınır
        std::wstring kartSahibi = JsonAlaniBul(veriJson, L"holder");
        std::wstring eklenmeZamani = JsonAlaniBul(veriJson, L"addedAt");
        OtomatikDoldurmaKasasi::KartSil(kartSahibi, eklenmeZamani);
        AyarlarIcinVeriGonder(sekmeId, L"cards"); // Güncel kart listesi geri gönderilir
    }
    else if (mesaj.rfind(L"reveal_card|", 0) == 0) // Kayıtlı bir kartın numarasının gösterilmesi isteniyorsa
    {
        std::wstring veriJson = mesaj.substr(12); // "reveal_card|" önekinden sonraki küçük JSON nesnesi alınır
        std::wstring kartSahibi = JsonAlaniBul(veriJson, L"holder");
        std::wstring eklenmeZamani = JsonAlaniBul(veriJson, L"addedAt");

        bool dogrulandi = Guvenlik::WindowsSifresiniKilitlenmeSuresiyleDogrula(m_pencere, Ayarlar::Oku().sifreKasasiKilitlenmeDakikasi); // Kart numarası gösterilmeden önce, aynı Windows kimlik doğrulaması istenir, şifre kasasıyla aynı kilitlenme süresini paylaşır

        std::wstring duzNumara;
        if (dogrulandi)
        {
            for (const KayitliKart& kayit : OtomatikDoldurmaKasasi::KartlariOku())
            {
                if (kayit.kartSahibi == kartSahibi && kayit.eklenmeZamani == eklenmeZamani)
                {
                    duzNumara = OtomatikDoldurmaKasasi::KartNumarasiniCoz(kayit.kartNumarasiSifreli);
                    break;
                }
            }
        }

        std::wstring cevap = L"{\"type\":\"card_revealed\",\"holder\":\"" + JsonIcinKacisla(kartSahibi) + L"\",\"addedAt\":\"" + JsonIcinKacisla(eklenmeZamani) + L"\",\"number\":" + (dogrulandi ? (L"\"" + JsonIcinKacisla(duzNumara) + L"\"") : L"null") + L"}";
        SekmeyeMesajGonder(sekmeId, cevap);
    }
}

// Bu fonksiyon, native taraftan toolbar arayüzüne bir metin mesajı gönderir
void BrowserWindow::ToolbaraMesajGonder(const std::wstring& mesaj)
{
    if (m_toolbarWebView != nullptr) // Toolbar WebView2'si henüz hazır değilse mesaj göndermeye çalışmak hataya sebep olur
    {
        m_toolbarWebView->PostWebMessageAsString(mesaj.c_str()); // Verilen metin, toolbar arayüzündeki JavaScript koduna düz bir metin mesajı olarak gönderilir
    }
}

// Bu fonksiyon, native taraftan hamburger menü paneline bir metin mesajı gönderir
void BrowserWindow::MenuyeMesajGonder(const std::wstring& mesaj)
{
    if (m_menuWebView != nullptr) // Menü WebView2'si henüz hazır değilse mesaj göndermeye çalışmak hataya sebep olur
    {
        m_menuWebView->PostWebMessageAsString(mesaj.c_str());
    }
}

// EnumChildWindows'a verilen geri çağırma, aranan pencereyi bulduğunda bu yapı üzerinden sonucu dışarı taşır
struct MenuPenceresiAramaBaglami
{
    RECT hedefAlaniEkranKoordinati; // Menü panelinin, ekran koordinatlarına çevrilmiş beklenen konum ve boyutu
    HWND bulunanPencere = nullptr; // Eşleşen pencere bulununca buraya yazılır
};

// Bu geri çağırma, pencerenin alt pencerelerini tek tek gezip WebView2'nin kendi iç pencere sınıfından olan ve konumu/boyutu menü paneliyle eşleşeni arar
static BOOL CALLBACK MenuPenceresiAra(HWND aday, LPARAM lp)
{
    MenuPenceresiAramaBaglami* baglam = reinterpret_cast<MenuPenceresiAramaBaglami*>(lp);

    wchar_t sinifAdi[64] = {};
    GetClassNameW(aday, sinifAdi, 64);
    if (wcsstr(sinifAdi, L"Chrome_WidgetWin") == nullptr) // WebView2'nin render ettiği her denetleyici, bu sınıf adını taşıyan kendi alt penceresini oluşturur
    {
        return TRUE; // Bu aday uygun sınıfta değil, aramaya devam edilir
    }

    RECT adayAlani;
    GetWindowRect(aday, &adayAlani); // Bu pencerenin EKRANDAKİ gerçek konumu ve boyutu alınır

    // Konum ve boyut, birkaç pikseli aşmayan bir toleransla karşılaştırılır; aynı anda birden fazla WebView2 denetleyicisi (toolbar, aktif sekme, devtools, menü) aynı ana pencerenin altında olduğu için sadece BOYUT değil KONUM da eşleşmelidir
    if (abs(adayAlani.left - baglam->hedefAlaniEkranKoordinati.left) <= 2 &&
        abs(adayAlani.top - baglam->hedefAlaniEkranKoordinati.top) <= 2 &&
        abs((adayAlani.right - adayAlani.left) - (baglam->hedefAlaniEkranKoordinati.right - baglam->hedefAlaniEkranKoordinati.left)) <= 2 &&
        abs((adayAlani.bottom - adayAlani.top) - (baglam->hedefAlaniEkranKoordinati.bottom - baglam->hedefAlaniEkranKoordinati.top)) <= 2)
    {
        baglam->bulunanPencere = aday;
        return FALSE; // Eşleşme bulundu, aramayı durdur
    }
    return TRUE; // Bu aday eşleşmedi, aramaya devam edilir
}

// Bu fonksiyon, menü panelinin WebView2'sinin gerçek alt penceresini bulup üzerine yuvarlak köşeli bir SetWindowRgn uygular
// menu.css'teki border-radius sadece ÇİZİMİ yuvarlatır, pencerenin GERÇEK dikdörtgen sınırını değiştirmez; bu yüzden köşelerde eski köşeli pencerenin kendisi görünmeye devam ederdi
// Bu fonksiyon o gerçek pencereyi de aynı yarıçapla kırparak, köşelerin dışında kalan kısmın hiç çizilmemesini (altında kalan içerik sekmesinin görünmesini) sağlar
void BrowserWindow::MenuPaneliKoseleriniYuvarlaYap(const RECT& menuAlaniIstemciKoordinati)
{
    int genislik = menuAlaniIstemciKoordinati.right - menuAlaniIstemciKoordinati.left;
    int yukseklik = menuAlaniIstemciKoordinati.bottom - menuAlaniIstemciKoordinati.top;
    if (genislik <= 0 || yukseklik <= 0) // Pencere o kadar küçültülmüş ki panel için hiç yer kalmamış, uygulanacak bir şey yok
    {
        return;
    }

    RECT hedefAlaniEkranKoordinati = menuAlaniIstemciKoordinati; // İstemci koordinatları, pencere penceresini aramak için ekran koordinatına çevrilecek
    MapWindowPoints(m_pencere, nullptr, reinterpret_cast<POINT*>(&hedefAlaniEkranKoordinati), 2); // İki köşe noktası (sol-üst, sağ-alt) tek seferde ekran koordinatına çevrilir

    MenuPenceresiAramaBaglami baglam;
    baglam.hedefAlaniEkranKoordinati = hedefAlaniEkranKoordinati;
    EnumChildWindows(m_pencere, MenuPenceresiAra, reinterpret_cast<LPARAM>(&baglam));
    if (baglam.bulunanPencere == nullptr) // Pencere henüz oluşmamış olabilir (örn. denetleyici daha yeni yaratıldı), bir dahaki boyut güncellemesinde tekrar denenecek
    {
        return;
    }

    UINT dpi = GetDpiForWindow(m_pencere); // Yarıçap, CSS pikseli olan menu.css'teki 14px ile görsel olarak örtüşsün diye güncel DPI ölçeğine göre büyütülür
    int yaricap = MulDiv(14, static_cast<int>(dpi), 96) * 2; // CreateRoundRectRgn, yarıçapı değil köşe ELİPSİNİN çapını ister, bu yüzden ikiyle çarpılır

    HRGN bolge = CreateRoundRectRgn(0, 0, genislik + 1, yukseklik + 1, yaricap, yaricap); // Sağ/alt sınır CreateRoundRectRgn'de hariç tutulduğu için +1 ile tam pencereyi kapsaması sağlanır
    SetWindowRgn(baglam.bulunanPencere, bolge, TRUE); // Bölge pencereye atanır (sahipliği pencereye geçer, ayrıca DeleteObject çağırmaya gerek yoktur) ve hemen yeniden çizilir
}

// Bu fonksiyon, hamburger menü panelini açar: boyutunu/konumunu hesaplayıp görünür yapar, güncel RAM ve yakınlaştırma bilgisini gönderir
void BrowserWindow::MenuyuAc()
{
    if (m_menuDenetleyici == nullptr) // Denetleyici henüz hazır değilse (program yeni açılmışsa) hiçbir şey yapılamaz
    {
        return;
    }

    m_hamburgerMenusuAcikMi = true; // Menünün açık olduğu işaretlenir
    BoyutlariGuncelle(); // Panelin konumu/boyutu, güncel pencere boyutuna göre hesaplanır
    m_menuDenetleyici->put_IsVisible(TRUE); // Panel görünür yapılır
    m_menuDenetleyici->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC); // Odak panele taşınır, böylece dışarı tıklanınca LostFocus ile otomatik kapanabilir

    // Bazen (WebView2'nin kendi birleştirme/compositing katmanındaki bir pürüzden dolayı) panel görünür yapıldığı anda henüz çizilmemiş oluyor ve o bölgede pencerenin arkası (masaüstü) görünüyor
    // Pencerenin tamamını, tüm alt WebView2 pencereleri dahil olmak üzere hemen yeniden çizmeye zorlamak, bu bayat (stale) görüntüyü düzeltip gerçek içeriğin boyanmasını tetikler
    RedrawWindow(m_pencere, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_ERASE);

    AyarVerisi gunceAyarlarMenu = Ayarlar::Oku(); // Panel her açıldığında güncel tema (özellikle açık/koyu mod) da gönderilir, aksi halde panel kendi ayrı sayfası olduğu için toolbar'daki moddan habersiz kalıp hep varsayılan koyu görünürdü
    MenuyeMesajGonder(L"{\"type\":\"theme\",\"theme\":" + TemaNesnesiJsonYap(gunceAyarlarMenu) + L"}");

    DWORD ramMB = ToplamRamKullanimiMB(); // Panel her açıldığında güncel RAM kullanımı hesaplanır
    MenuyeMesajGonder(L"{\"type\":\"ramstatus\",\"ramMB\":" + std::to_wstring(ramMB) + L"}"); // ve panele gönderilir

    SekmeBilgisi* aktifSekme = AktifSekmeyiAl(); // Panel her açıldığında güncel yakınlaştırma yüzdesi de gönderilir
    if (aktifSekme != nullptr && aktifSekme->denetleyici != nullptr)
    {
        double yakinlastirmaOrani = 1.0;
        aktifSekme->denetleyici->get_ZoomFactor(&yakinlastirmaOrani);
        int yakinlastirmaYuzdesi = static_cast<int>(yakinlastirmaOrani * 100.0 + 0.5);
        MenuyeMesajGonder(L"{\"type\":\"zoom\",\"zoomPercent\":" + std::to_wstring(yakinlastirmaYuzdesi) + L"}");
    }
}

// Bu fonksiyon, hamburger menü panelini kapatır (gizler)
void BrowserWindow::MenuyuKapat()
{
    if (!m_hamburgerMenusuAcikMi) // Zaten kapalıysa
    {
        return; // Tekrar kapatmaya gerek yok
    }
    m_hamburgerMenusuAcikMi = false; // Menünün kapalı olduğu işaretlenir
    m_menuSonKapanmaZamani = GetTickCount64(); // Bu anın kaydedilmesi, hamburger butonuna tıklamanın tetiklediği LostFocus'tan hemen sonra gelecek togglemenu mesajının menüyü yanlışlıkla tekrar açmasını engeller
    if (m_menuDenetleyici != nullptr)
    {
        m_menuDenetleyici->put_IsVisible(FALSE); // Panel gizlenir
    }
}

// Bu fonksiyon, verilen yerel (127.0.0.1) bağlantı noktasına, verilen yola basit bir HTTP GET isteği atar ve yanıt gövdesini ham baytlar olarak döndürür
// Bu çağrı SENKRONDUR (sonucu beklerken arayüz donar), bu normalde kaçınılması gereken bir şeydir, ama burada istek sadece bu bilgisayarın kendi içinde (127.0.0.1) olduğu ve çok kısa bir zaman aşımıyla sınırlandığı için pratikte fark edilmeyecek kadar hızlı tamamlanır
static std::string YerelHttpGet(int port, const std::wstring& yol, int zamanAsimiMs)
{
    std::string sonuc; // Yanıt gövdesinin biriktirileceği metin

    HINTERNET oturum = WinHttpOpen(L"Tarayici DevTools", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0); // Bu tek istek için geçici bir WinHTTP oturumu açılır
    if (oturum == nullptr) { return sonuc; }
    WinHttpSetTimeouts(oturum, zamanAsimiMs, zamanAsimiMs, zamanAsimiMs, zamanAsimiMs); // Çözümleme, bağlanma, gönderme ve alma aşamalarının hepsine aynı kısa zaman aşımı uygulanır

    HINTERNET baglanti = WinHttpConnect(oturum, L"127.0.0.1", static_cast<INTERNET_PORT>(port), 0); // Sadece bu bilgisayarın kendisine bağlanılır
    if (baglanti != nullptr)
    {
        HINTERNET istek = WinHttpOpenRequest(baglanti, L"GET", yol.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (istek != nullptr)
        {
            if (WinHttpSendRequest(istek, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(istek, nullptr)) // İstek gönderilir ve başlıklar beklenir
            {
                DWORD mevcutBoyut = 0; // Bir sonraki okunabilir veri parçasının boyutu
                do
                {
                    mevcutBoyut = 0;
                    if (!WinHttpQueryDataAvailable(istek, &mevcutBoyut) || mevcutBoyut == 0) { break; } // Daha fazla veri yoksa döngüden çıkılır
                    std::vector<char> arabellek(mevcutBoyut); // Bu parça için bir arabellek ayrılır
                    DWORD okunanBayt = 0;
                    if (WinHttpReadData(istek, arabellek.data(), mevcutBoyut, &okunanBayt)) // Parça okunur
                    {
                        sonuc.append(arabellek.data(), okunanBayt); // Okunan parça, toplam yanıta eklenir
                    }
                } while (mevcutBoyut > 0);
            }
            WinHttpCloseHandle(istek);
        }
        WinHttpCloseHandle(baglanti);
    }
    WinHttpCloseHandle(oturum);

    return sonuc; // Toplanan yanıt gövdesi (başarısızlık durumunda boş) döndürülür
}

// Bu fonksiyon, F12 tuşuna basıldığında çağrılır, DevTools paneli kapalıysa açar, açıksa kapatır
void BrowserWindow::DevToolsAcKapat()
{
    if (m_devToolsDenetleyici == nullptr) // Denetleyici henüz hazır değilse (program yeni açılmışsa) hiçbir şey yapılamaz
    {
        return;
    }

    if (m_devToolsAcikMi) // Panel açıksa, bu basış onu kapatma isteğidir
    {
        m_devToolsAcikMi = false;
        m_devToolsDenetleyici->put_IsVisible(FALSE); // Panel gizlenir
        m_devToolsWebView->Navigate(L"about:blank"); // İçerik boşaltılır, böylece panel bir dahaki açılışa kadar eski bir sekmenin hata ayıklama bağlantısını açık tutmaz
        BoyutlariGuncelle(); // Aktif sekmenin içerik alanı, panelin bıraktığı boşluğu geri kaplayacak şekilde genişletilir
        return;
    }

    SekmeBilgisi* aktifSekme = AktifSekmeyiAl(); // DevTools, her zaman o an ekranda görünen aktif sekmeye bağlanır
    if (aktifSekme == nullptr || aktifSekme->webView == nullptr) // Bağlanacak bir sekme yoksa (örneğin program daha yeni açılmışsa) paneli açmanın bir anlamı yoktur
    {
        return;
    }

    LPWSTR guncelAdresPtr = nullptr; // Aktif sekmenin güncel adresini tutacak değişken
    aktifSekme->webView->get_Source(&guncelAdresPtr);
    std::wstring aktifAdres = guncelAdresPtr ? guncelAdresPtr : L""; // Adres, güvenli bir metin değişkenine aktarılır
    if (guncelAdresPtr != nullptr) { CoTaskMemFree(guncelAdresPtr); }

    m_devToolsAcikMi = true; // Kullanıcının isteği hemen işaretlenir, böylece F12'ye art arda basılırsa ikinci basış "kapatma" dalına doğru şekilde girer
    BoyutlariGuncelle(); // Panel hemen görünür boşluğa yerleştirilir, aktif sekmenin içerik alanı daraltılır
    m_devToolsDenetleyici->put_IsVisible(TRUE); // Panel, hedef bulma işlemi arka planda sürerken bile hemen görünür yapılır, böylece kullanıcı F12'nin tepki verdiğini anında görür (içi bir süre boş kalabilir)

    // Uzaktan hata ayıklama sunucusuna HTTP isteği atmak, teorik olarak kısa sürse de UI iş parçacığını BLOKE edebilir (ağ yığını, güvenlik yazılımı veya sistem yükü yüzünden)
    // Bu yüzden istek ve JSON tarama işlemi tamamen AYRI bir iş parçacığında yapılır, WebView2/COM nesnelerine o iş parçacığından HİÇ dokunulmaz, sadece sonuç bulununca ana pencereye özel bir mesajla (DevToolsHedefBulunduMesaji) haber verilir
    HWND pencere = m_pencere;
    int port = DevToolsUzaktanHataAyiklamaPortu;
    std::shared_ptr<bool> gecerlilikBayragi = m_pencereGecerliMi;
    std::thread([pencere, port, aktifAdres, gecerlilikBayragi]()
    {
        std::string hamYanit = YerelHttpGet(port, L"/json/list", 2000); // Artık UI'yi bloklamadığı için, geçici bir yavaşlamayı tolere edebilecek daha cömert bir zaman aşımı kullanılabilir
        std::wstring yanit = Utf8iGenisMetneCevir(hamYanit); // Yanıt, UTF-8 olduğu varsayılarak geniş metne çevrilir

        std::wstring hedefKimligi; // Eşleşen hedefin kimlik numarası (id alanı) bulununca buraya yazılır
        size_t konum = 0;
        while (true) // Yanıttaki her JSON nesnesi (her biri bir sekmeyi/hedefi temsil eder) sırayla taranır
        {
            size_t baslangic = yanit.find(L'{', konum);
            if (baslangic == std::wstring::npos) { break; } // Daha fazla hedef yoksa döngüden çıkılır
            size_t bitis = yanit.find(L'}', baslangic);
            if (bitis == std::wstring::npos) { break; }

            std::wstring nesne = yanit.substr(baslangic, bitis - baslangic + 1); // Bu tek hedefin tam JSON metni ayıklanır
            // Sadece normal bir sayfa hedefi (arka plan sayfası, servis çalışanı gibi diğer hedef türleri değil) ve adresi aktif sekmenin adresiyle TAM eşleşen hedef kabul edilir
            if (JsonAlaniBul(nesne, L"type") == L"page" && JsonAlaniBul(nesne, L"url") == aktifAdres)
            {
                hedefKimligi = JsonAlaniBul(nesne, L"id");
                break;
            }
            konum = bitis + 1;
        }

        // Yanıttaki "devtoolsFrontendUrl" alanı KULLANILMAZ: bu alan Microsoft'un kısa bağlantı servisine (aka.ms) işaret ediyor ve denendiğinde bozuk çıkıp Bing arama sayfasına yönlendiriyor
        // Bunun yerine, hata ayıklama sunucusunun KENDİSİNİN de aynı DevTools arayüzünü doğrudan (hiçbir dış bağlantıya ihtiyaç duymadan) sunduğu doğrulandı, bu yüzden adres, hedefin kimlik numarasından kendi elimizle inşa edilir
        std::wstring devToolsOnYuzAdresi; // Panelin gezinileceği, tamamen yerel DevTools arayüz adresi
        if (!hedefKimligi.empty())
        {
            devToolsOnYuzAdresi = L"http://127.0.0.1:" + std::to_wstring(port) + L"/devtools/inspector.html?ws=127.0.0.1:" + std::to_wstring(port) + L"/devtools/page/" + hedefKimligi;
        }

        if (!*gecerlilikBayragi) // Bu arka plan işlemi sürerken pencere kapatılmaya başladıysa, ana pencereye mesaj göndermeye bile gerek yoktur
        {
            return;
        }

        std::wstring* sonucIsaretcisi = new std::wstring(devToolsOnYuzAdresi); // Mesajın LPARAM'ında taşınacak, sahipliği mesajı işleyen tarafa geçen bir işaretçi (hedef bulunamadıysa boş bir metin işaret eder)
        PostMessageW(pencere, DevToolsHedefBulunduMesaji, 0, reinterpret_cast<LPARAM>(sonucIsaretcisi)); // Sonuç, ana pencerenin mesaj kuyruğuna (ve dolayısıyla ana iş parçacığına) güvenle teslim edilir
    }).detach(); // İş parçacığı, programın geri kalanını beklemeden arka planda bağımsız çalışır, tamamlandığında kaynakları kendiliğinden serbest bırakır
}

// Bu fonksiyon, gösterilecek bekleyen bir izin isteği varsa panelini o isteğin bilgileriyle gösterir, hiç bekleyen istek yoksa paneli gizler
// "en eski" bekleyen istek gösterilir, m_bekleyenIzinIstekleri bir std::map olduğu ve kimlik numaraları sırayla arttığı için, begin() her zaman en eski (ilk gelen) isteği verir
void BrowserWindow::IzinBildirimPanelindekiIstegiGuncelle()
{
    if (m_izinDenetleyici == nullptr) // Denetleyici henüz hazır değilse hiçbir şey yapılamaz
    {
        return;
    }

    if (m_bekleyenIzinIstekleri.empty()) // Gösterilecek bekleyen bir istek yoksa
    {
        m_izinDenetleyici->put_IsVisible(FALSE); // Panel gizlenir
        return;
    }

    auto& [istekId, istek] = *m_bekleyenIzinIstekleri.begin(); // En eski bekleyen istek alınır

    LPWSTR kaynakPtr = nullptr; // İsteği yapan sitenin kaynağını tutacak değişken
    istek.argumanlar->get_Uri(&kaynakPtr);
    std::wstring kaynak = kaynakPtr ? kaynakPtr : L"";
    if (kaynakPtr != nullptr) { CoTaskMemFree(kaynakPtr); }

    COREWEBVIEW2_PERMISSION_KIND tur; // İstenen izin türü
    istek.argumanlar->get_PermissionKind(&tur);
    std::wstring turMetni = IzinTuruMetne(tur);

    std::wstring mesaj = L"{\"type\":\"permissionrequest\",\"requestId\":" + std::to_wstring(istekId) + L",\"origin\":\"" + JsonIcinKacisla(kaynak) + L"\",\"kind\":\"" + turMetni + L"\"}";
    m_izinWebView->PostWebMessageAsString(mesaj.c_str()); // İsteğin bilgileri, panele gönderilir

    BoyutlariGuncelle(); // Panelin konumu hesaplanır
    m_izinDenetleyici->put_IsVisible(TRUE); // Panel görünür yapılır
}

// Bu fonksiyon, izin panelindeki "İzin Ver" veya "Engelle" butonuna tıklandığında çağrılır
void BrowserWindow::IzinIstegiYanitla(int istekId, bool izinVerildi, bool hatirla)
{
    auto bulunan = m_bekleyenIzinIstekleri.find(istekId); // Yanıtlanan istek, bekleyenler listesinde aranır
    if (bulunan == m_bekleyenIzinIstekleri.end()) // Eğer bu kimlikte bir istek bulunamazsa (örneğin sekme bu arada kapatılıp istek zaten temizlenmişse)
    {
        return; // Yapılacak bir şey yoktur
    }

    BekleyenIzinIstegi istek = bulunan->second; // İsteğin bir kopyası alınır, map'ten silinmeden önce
    m_bekleyenIzinIstekleri.erase(bulunan); // İstek, bekleyenler listesinden çıkarılır

    if (hatirla) // Kullanıcı "Hatırla" seçeneğini işaretlediyse
    {
        LPWSTR kaynakPtr = nullptr;
        istek.argumanlar->get_Uri(&kaynakPtr);
        std::wstring kaynak = kaynakPtr ? kaynakPtr : L"";
        if (kaynakPtr != nullptr) { CoTaskMemFree(kaynakPtr); }

        COREWEBVIEW2_PERMISSION_KIND tur;
        istek.argumanlar->get_PermissionKind(&tur);
        IzinKasasi::KararKaydet(kaynak, IzinTuruMetne(tur), izinVerildi); // Karar, bir sonraki istekte tekrar sorulmaması için kalıcı olarak kaydedilir
    }

    istek.argumanlar->put_State(izinVerildi ? COREWEBVIEW2_PERMISSION_STATE_ALLOW : COREWEBVIEW2_PERMISSION_STATE_DENY); // Karar, bekleyen siteye bildirilir
    istek.erteleme->Complete(); // Erteleme tamamlanır, site artık bekletilmeyi bırakıp karara göre devam eder

    IzinBildirimPanelindekiIstegiGuncelle(); // Sırada başka bir bekleyen istek varsa o gösterilir, yoksa panel gizlenir
}

// Bu fonksiyon, site bilgisi panelini aktif sekmenin bilgileriyle açar
void BrowserWindow::SiteBilgisiPaneliniAc()
{
    if (m_sitePaneliDenetleyici == nullptr) // Denetleyici henüz hazır değilse (program yeni açılmışsa) hiçbir şey yapılamaz
    {
        return;
    }

    SekmeBilgisi* aktifSekme = AktifSekmeyiAl(); // Panel, her zaman o an ekranda görünen aktif sekmenin sitesine ait bilgileri gösterir
    if (aktifSekme == nullptr || aktifSekme->webView == nullptr) // Gösterilecek bir sekme yoksa (örneğin program daha yeni açılmışsa) paneli açmanın bir anlamı yoktur
    {
        return;
    }

    LPWSTR guncelAdresPtr = nullptr; // Aktif sekmenin güncel adresini tutacak değişken
    aktifSekme->webView->get_Source(&guncelAdresPtr);
    m_sitePaneliKaynagi = guncelAdresPtr ? guncelAdresPtr : L""; // Panelin hangi siteye ait olduğu, native tarafta hatırlanır, "izin kaldır" komutu bu hatırlanan kaynağa uygulanır
    if (guncelAdresPtr != nullptr) { CoTaskMemFree(guncelAdresPtr); }

    m_sitePaneliAcikMi = true; // Panelin açık olduğu işaretlenir
    BoyutlariGuncelle(); // Panelin konumu/boyutu, güncel pencere boyutuna göre hesaplanır
    m_sitePaneliDenetleyici->put_IsVisible(TRUE); // Panel görünür yapılır
    m_sitePaneliDenetleyici->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC); // Odak panele taşınır, böylece dışarı tıklanınca LostFocus ile otomatik kapanabilir

    // Hamburger menüsünde de karşılaşılan, panelin görünür yapıldığı anda henüz çizilmemiş olma pürüzüne karşı aynı önlem
    RedrawWindow(m_pencere, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_ERASE);

    SiteBilgisiPaneliVeriGonder(); // Panelin içeriği (güvenlik durumu, izin listesi) gönderilir
}

// Bu fonksiyon, site bilgisi panelini kapatır (gizler)
void BrowserWindow::SiteBilgisiPaneliniKapat()
{
    if (!m_sitePaneliAcikMi) // Zaten kapalıysa
    {
        return; // Tekrar kapatmaya gerek yok
    }
    m_sitePaneliAcikMi = false; // Panelin kapalı olduğu işaretlenir
    m_sitePaneliSonKapanmaZamani = GetTickCount64(); // Kilit ikonuna tıklamanın tetiklediği LostFocus'tan hemen sonra gelecek togglesitepaneli mesajının paneli yanlışlıkla tekrar açmasını engeller
    m_sitePaneliKaynagi.clear(); // Panel kapandığına göre, artık hiçbir siteye ait olarak gösterilmiyor
    if (m_sitePaneliDenetleyici != nullptr)
    {
        m_sitePaneliDenetleyici->put_IsVisible(FALSE); // Panel gizlenir
    }
}

// Bu fonksiyon, panelde gösterilen sitenin güvenlik durumunu ve bu siteye ait kayıtlı izin kararlarını JSON olarak panele gönderir
void BrowserWindow::SiteBilgisiPaneliVeriGonder()
{
    if (m_sitePaneliWebView == nullptr) // WebView2'si henüz hazır değilse mesaj göndermeye çalışmak hataya sebep olur
    {
        return;
    }

    bool dahiliMi = (m_sitePaneliKaynagi.rfind(L"https://tarayici.local", 0) == 0); // Bu sitenin, programın kendi dahili bir sayfası olup olmadığı kontrol edilir
    bool guvenliMi = (m_sitePaneliKaynagi.rfind(L"https://", 0) == 0); // Adresin https ile başlayıp başlamadığı kontrol edilir, dahili sayfalar da https şemasını kullandığı için zaten güvenli sayılır

    SekmeBilgisi* aktifSekme = AktifSekmeyiAl(); // Panel her zaman aktif sekmeye ait olduğu için, sertifika detayları da oradan okunur

    std::vector<IzinKaydi> tumKararlar = IzinKasasi::TumKararlariOku(); // Kayıtlı tüm izin kararları okunur
    std::wstring izinlerMetni = L"["; // Sadece bu siteye ait olanlar, bir JSON dizisi olarak hazırlanır
    bool ilkKayitMi = true;
    for (const IzinKaydi& kayit : tumKararlar) // Her kayıt sırayla kontrol edilir
    {
        if (kayit.kaynak != m_sitePaneliKaynagi) // Bu kayıt, panelde gösterilen siteye ait değilse
        {
            continue; // Atlanır
        }
        if (!ilkKayitMi) { izinlerMetni += L","; }
        izinlerMetni += L"{\"tur\":\"" + JsonIcinKacisla(kayit.tur) + L"\",\"izinVerildi\":" + (kayit.izinVerildi ? L"true" : L"false") + L"}";
        ilkKayitMi = false;
    }
    izinlerMetni += L"]";

    std::wstring sertifikaMetni = L"null"; // Sertifika henüz yakalanmadıysa (CDP olayı gelmediyse) veya sekme yoksa panel detay göstermez
    if (aktifSekme != nullptr && !aktifSekme->sertifikaKonu.empty())
    {
        sertifikaMetni = L"{\"subject\":\"" + JsonIcinKacisla(aktifSekme->sertifikaKonu) + L"\",\"issuer\":\"" + JsonIcinKacisla(aktifSekme->sertifikaYayinlayan) + L"\",\"protocol\":\"" + JsonIcinKacisla(aktifSekme->sertifikaProtokol) + L"\"}";
    }

    std::wstring mesaj = L"{\"type\":\"sitebilgisi\",\"origin\":\"" + JsonIcinKacisla(m_sitePaneliKaynagi) + L"\",\"secure\":" + (guvenliMi ? L"true" : L"false") + L",\"internal\":" + (dahiliMi ? L"true" : L"false") + L",\"certificate\":" + sertifikaMetni + L",\"permissions\":" + izinlerMetni + L"}";
    m_sitePaneliWebView->PostWebMessageAsString(mesaj.c_str()); // Hazırlanan bilgi panele gönderilir
}

// Bu fonksiyon, adres çubuğuna yazılırken gelen öneri isteğini işler, geçmiş ve yer imlerinden eşleşenleri bulup paneli konumlandırıp gösterir
// Bu küçük yardımcı, bir aritmetik ifadeyi (sadece sayılar, +, -, *, /, (, ) ve ondalık nokta/virgül) güvenle hesaplar
// eval() veya benzeri bir şey KULLANILMAZ, bunun yerine klasik bir özyinelemeli iniş (recursive descent) ayrıştırıcı yazılır, böylece rastgele kod çalıştırma riski olmaz
// Ayrıştırma başarısız olursa (ifade bir matematik ifadesi gibi görünmüyorsa) basariliMi false olarak ayarlanır
static double AritmetikIfadeyiHesapla(const std::wstring& ifade, size_t& konum, bool& basariliMi);

static double AritmetikSayiOku(const std::wstring& ifade, size_t& konum, bool& basariliMi)
{
    while (konum < ifade.size() && ifade[konum] == L' ') { konum++; } // Boşluklar atlanır

    if (konum < ifade.size() && ifade[konum] == L'(') // Parantez açılmışsa, içindeki ifade özyinelemeli olarak hesaplanır
    {
        konum++; // Açılış parantezi atlanır
        double sonuc = AritmetikIfadeyiHesapla(ifade, konum, basariliMi);
        while (konum < ifade.size() && ifade[konum] == L' ') { konum++; }
        if (konum >= ifade.size() || ifade[konum] != L')') { basariliMi = false; return 0; } // Kapanış parantezi bulunamadı
        konum++; // Kapanış parantezi atlanır
        return sonuc;
    }

    size_t baslangic = konum;
    while (konum < ifade.size() && (iswdigit(ifade[konum]) || ifade[konum] == L'.' || ifade[konum] == L',')) { konum++; } // Ardışık rakamlar ve tek bir ondalık ayraç okunur
    if (konum == baslangic) { basariliMi = false; return 0; } // Hiç rakam okunamadıysa geçerli bir sayı yoktur

    std::wstring sayiMetni = ifade.substr(baslangic, konum - baslangic);
    for (wchar_t& harf : sayiMetni) { if (harf == L',') { harf = L'.'; } } // Türkçe ondalık virgülü, çözümleme için noktaya çevrilir
    try { return std::stod(sayiMetni); } catch (...) { basariliMi = false; return 0; }
}

static double AritmetikCarpimBolumHesapla(const std::wstring& ifade, size_t& konum, bool& basariliMi)
{
    double sonuc = AritmetikSayiOku(ifade, konum, basariliMi);
    while (basariliMi)
    {
        while (konum < ifade.size() && ifade[konum] == L' ') { konum++; }
        if (konum >= ifade.size() || (ifade[konum] != L'*' && ifade[konum] != L'/' && ifade[konum] != L'x')) { break; }
        wchar_t islemTuru = ifade[konum];
        konum++;
        double sagTaraf = AritmetikSayiOku(ifade, konum, basariliMi);
        if (!basariliMi) { break; }
        if (islemTuru == L'/')
        {
            if (sagTaraf == 0) { basariliMi = false; break; } // Sıfıra bölme engellenir
            sonuc /= sagTaraf;
        }
        else { sonuc *= sagTaraf; }
    }
    return sonuc;
}

static double AritmetikIfadeyiHesapla(const std::wstring& ifade, size_t& konum, bool& basariliMi)
{
    double sonuc = AritmetikCarpimBolumHesapla(ifade, konum, basariliMi);
    while (basariliMi)
    {
        while (konum < ifade.size() && ifade[konum] == L' ') { konum++; }
        if (konum >= ifade.size() || (ifade[konum] != L'+' && ifade[konum] != L'-')) { break; }
        wchar_t islemTuru = ifade[konum];
        konum++;
        double sagTaraf = AritmetikCarpimBolumHesapla(ifade, konum, basariliMi);
        if (!basariliMi) { break; }
        sonuc = (islemTuru == L'+') ? (sonuc + sagTaraf) : (sonuc - sagTaraf);
    }
    return sonuc;
}

// Bu fonksiyon, verilen bir sayıyı, gereksiz ondalık sıfırlar olmadan okunabilir bir metne çevirir
static std::wstring SayiyiMetneCevir(double deger)
{
    wchar_t tampon[64];
    swprintf_s(tampon, L"%.4f", deger);
    std::wstring metin = tampon;
    size_t noktaKonumu = metin.find(L'.');
    if (noktaKonumu != std::wstring::npos) // Ondalık kısımdaki gereksiz sondaki sıfırlar temizlenir
    {
        size_t sonSifirOlmayan = metin.find_last_not_of(L'0');
        if (sonSifirOlmayan == noktaKonumu) { sonSifirOlmayan--; } // Ondalık kısmın tamamı sıfırsa, noktanın kendisi de kaldırılır
        metin = metin.substr(0, sonSifirOlmayan + 1);
    }
    return metin;
}

// Bu fonksiyon, adres çubuğuna yazılan metnin bir hesap makinesi ifadesi veya birim çevirme isteği olup olmadığını kontrol eder
// Öyleyse "ifade = sonuç" biçiminde okunabilir bir sonuç metni döndürür, değilse boş metin döndürür
static std::wstring HesapMakinesiVeyaBirimCeviriciSonucu(const std::wstring& sorgu)
{
    std::wstring sorguKucuk;
    for (wchar_t harf : sorgu) { sorguKucuk += static_cast<wchar_t>(towlower(harf)); }

    // Birim çevirici: "<sayı> <birim1> to <birim2>" veya "<sayı><birim1>to<birim2>" biçimini arar
    size_t toKonumu = sorguKucuk.find(L" to ");
    if (toKonumu != std::wstring::npos)
    {
        std::wstring solTaraf = sorguKucuk.substr(0, toKonumu);
        std::wstring hedefBirim = sorguKucuk.substr(toKonumu + 4);
        // Sondaki/baştaki boşluklar temizlenir
        while (!hedefBirim.empty() && hedefBirim.front() == L' ') { hedefBirim.erase(0, 1); }
        while (!hedefBirim.empty() && hedefBirim.back() == L' ') { hedefBirim.pop_back(); }

        size_t sayiSonu = 0; // Sol taraftaki sayının bittiği, birimin başladığı konum bulunur
        while (sayiSonu < solTaraf.size() && (iswdigit(solTaraf[sayiSonu]) || solTaraf[sayiSonu] == L'.' || solTaraf[sayiSonu] == L',' || solTaraf[sayiSonu] == L' ')) { sayiSonu++; }
        if (sayiSonu == 0) { return L""; } // Sayı yoksa birim çevirisi değildir

        std::wstring sayiMetni = solTaraf.substr(0, sayiSonu);
        for (wchar_t& harf : sayiMetni) { if (harf == L',') { harf = L'.'; } }
        std::wstring kaynakBirim = solTaraf.substr(sayiSonu);
        while (!kaynakBirim.empty() && kaynakBirim.front() == L' ') { kaynakBirim.erase(0, 1); }

        double deger = 0;
        try { deger = std::stod(sayiMetni); } catch (...) { return L""; }

        // En yaygın birimler için sabit çarpanlar, hepsi "temel birime çevir, sonra hedefe çevir" mantığıyla çalışır
        struct BirimTanimi { const wchar_t* ad; double temelBirimeCarpan; };
        static const BirimTanimi uzunlukBirimleri[] = { {L"km", 1000.0}, {L"m", 1.0}, {L"cm", 0.01}, {L"mi", 1609.344}, {L"mile", 1609.344}, {L"miles", 1609.344}, {L"ft", 0.3048}, {L"feet", 0.3048}, {L"in", 0.0254}, {L"inch", 0.0254} };
        static const BirimTanimi agirlikBirimleri[] = { {L"kg", 1.0}, {L"g", 0.001}, {L"lb", 0.45359237}, {L"lbs", 0.45359237}, {L"pound", 0.45359237}, {L"pounds", 0.45359237}, {L"oz", 0.028349523125} };

        auto birimDiziIcindeAra = [](const BirimTanimi* dizi, size_t sayac, const std::wstring& ad) -> const BirimTanimi*
        {
            for (size_t i = 0; i < sayac; i++) { if (dizi[i].ad == ad) { return &dizi[i]; } }
            return nullptr;
        };

        // Sıcaklık, doğrusal çarpanla ifade edilemediği için ayrıca özel ele alınır
        bool kaynakCelsius = (kaynakBirim == L"c" || kaynakBirim == L"celsius");
        bool kaynakFahrenheit = (kaynakBirim == L"f" || kaynakBirim == L"fahrenheit");
        bool hedefCelsius = (hedefBirim == L"c" || hedefBirim == L"celsius");
        bool hedefFahrenheit = (hedefBirim == L"f" || hedefBirim == L"fahrenheit");
        if ((kaynakCelsius || kaynakFahrenheit) && (hedefCelsius || hedefFahrenheit))
        {
            double sonuc;
            if (kaynakCelsius && hedefFahrenheit) { sonuc = deger * 9.0 / 5.0 + 32.0; }
            else if (kaynakFahrenheit && hedefCelsius) { sonuc = (deger - 32.0) * 5.0 / 9.0; }
            else { sonuc = deger; } // Aynı birimden aynı birime
            return sorgu + L" = " + SayiyiMetneCevir(sonuc) + L" " + hedefBirim;
        }

        const BirimTanimi* kaynak = birimDiziIcindeAra(uzunlukBirimleri, sizeof(uzunlukBirimleri) / sizeof(uzunlukBirimleri[0]), kaynakBirim);
        const BirimTanimi* hedef = birimDiziIcindeAra(uzunlukBirimleri, sizeof(uzunlukBirimleri) / sizeof(uzunlukBirimleri[0]), hedefBirim);
        if (kaynak == nullptr || hedef == nullptr) // Uzunlukta bulunamadıysa, ağırlık birimlerinde aranır
        {
            kaynak = birimDiziIcindeAra(agirlikBirimleri, sizeof(agirlikBirimleri) / sizeof(agirlikBirimleri[0]), kaynakBirim);
            hedef = birimDiziIcindeAra(agirlikBirimleri, sizeof(agirlikBirimleri) / sizeof(agirlikBirimleri[0]), hedefBirim);
        }
        if (kaynak == nullptr || hedef == nullptr) { return L""; } // İkisi de bulunamadıysa, tanınan bir birim çevirisi değildir

        double sonuc = deger * kaynak->temelBirimeCarpan / hedef->temelBirimeCarpan;
        return sorgu + L" = " + SayiyiMetneCevir(sonuc) + L" " + hedefBirim;
    }

    // Hesap makinesi: ifade en az bir işlem karakteri içermiyorsa (örn. tek bir kelime veya sayı), yanlışlıkla her adres çubuğu girdisini "hesaplamaya" çalışmamak için burada durulur
    bool islemKarakteriVarMi = false;
    bool rakamVarMi = false;
    for (wchar_t harf : sorgu)
    {
        if (harf == L'+' || harf == L'-' || harf == L'*' || harf == L'/' || harf == L'x') { islemKarakteriVarMi = true; }
        else if (iswdigit(harf)) { rakamVarMi = true; }
        else if (harf != L' ' && harf != L'.' && harf != L',' && harf != L'(' && harf != L')') { return L""; } // Tanınmayan bir karakter varsa (harf, nokta işareti gibi), bu bir matematik ifadesi değildir
    }
    if (!islemKarakteriVarMi || !rakamVarMi) { return L""; } // En az bir işlem ve bir rakam olmalı, aksi halde adres çubuğuna yazılan normal bir arama/adres olabilir

    size_t konum = 0;
    bool basariliMi = true;
    double sonuc = AritmetikIfadeyiHesapla(sorgu, konum, basariliMi);
    while (konum < sorgu.size() && sorgu[konum] == L' ') { konum++; } // Sondaki boşluklar atlanır
    if (!basariliMi || konum != sorgu.size()) { return L""; } // İfadenin tamamı başarıyla ve eksiksiz ayrıştırılamadıysa geçerli değildir

    return sorgu + L" = " + SayiyiMetneCevir(sonuc);
}

void BrowserWindow::AdresOnerileriniGosterVeGonder(const std::wstring& mesaj)
{
    if (m_oneriDenetleyici == nullptr) // Panel henüz hazır değilse gösterilecek bir şey yoktur
    {
        return;
    }

    std::wstring sorgu = JsonAlaniBul(mesaj, L"query"); // Kullanıcının adres çubuğuna yazdığı metin
    int sol = _wtoi(JsonAlaniBul(mesaj, L"left").c_str());
    int ust = _wtoi(JsonAlaniBul(mesaj, L"top").c_str());
    int genislik = _wtoi(JsonAlaniBul(mesaj, L"width").c_str());

    std::wstring sorguKucuk; // Karşılaştırma büyük/küçük harf duyarsız yapılsın diye, sorgunun küçük harfli hali hazırlanır
    for (wchar_t harf : sorgu) { sorguKucuk += static_cast<wchar_t>(towlower(harf)); }

    struct OneriKaydi { std::wstring adres; std::wstring baslik; };
    std::vector<OneriKaydi> oneriler; // Eşleşen kayıtlar, en fazla altı adet olacak şekilde burada toplanır
    std::vector<std::wstring> gorulenAdresler; // Aynı adresin (örneğin hem geçmişte hem yer imlerinde olduğu için) iki kez önerilmesini engellemek için

    auto adayEkle = [&](const std::wstring& adres, const std::wstring& baslik) // Bu küçük yardımcı, bir kaydın sorguya uyup uymadığını kontrol edip, uyuyorsa (ve tekrar değilse) listeye ekler
    {
        if (oneriler.size() >= 6) { return; } // Panel, çok uzun olmasın diye en fazla altı öneri gösterir
        std::wstring adresKucuk, baslikKucuk;
        for (wchar_t harf : adres) { adresKucuk += static_cast<wchar_t>(towlower(harf)); }
        for (wchar_t harf : baslik) { baslikKucuk += static_cast<wchar_t>(towlower(harf)); }
        if (adresKucuk.find(sorguKucuk) == std::wstring::npos && baslikKucuk.find(sorguKucuk) == std::wstring::npos) { return; } // Ne adreste ne başlıkta sorgu geçiyorsa aday değildir
        if (std::find(gorulenAdresler.begin(), gorulenAdresler.end(), adres) != gorulenAdresler.end()) { return; } // Bu adres zaten eklenmişse tekrar eklenmez
        gorulenAdresler.push_back(adres);
        oneriler.push_back({ adres, baslik });
    };

    std::wstring hesapSonucu = HesapMakinesiVeyaBirimCeviriciSonucu(sorgu); // Sorgu bir matematik ifadesi veya birim çevirisiyse, sonucu hesaplanır, değilse boş döner

    std::vector<SayfaKaydi> yerimleri = Depolama::YerImleriniOku(); // Yer imleri, geçmişten önce kontrol edilir, kullanıcının bilerek kaydettiği sayfalar önceliklidir
    for (const SayfaKaydi& kayit : yerimleri) { adayEkle(kayit.adres, kayit.baslik); }

    std::vector<SayfaKaydi> gecmis = Depolama::GecmisiOku(); // Geçmiş, en yeniden en eskiye doğru gezilir, en son ziyaret edilenler önceliklidir
    for (size_t i = gecmis.size(); i-- > 0; ) { adayEkle(gecmis[i].adres, gecmis[i].baslik); }

    if (oneriler.empty() && hesapSonucu.empty()) // Hiç eşleşme ve hiç hesap sonucu yoksa, panelin gösterilecek bir şeyi yoktur
    {
        OneriPaneliniKapat();
        return;
    }

    m_oneriSecilenIndeks = -1; // Liste her yenilendiğinde, önceki yazımdan kalan klavye seçimi geçersizdir
    m_oneriSeciliAdresler.clear();
    for (const OneriKaydi& kayit : oneriler) { m_oneriSeciliAdresler.push_back(kayit.adres); } // Hesap sonucu satırı hariç, sırayla her satırın adresi burada tutulur

    int toplamSatirSayisi = static_cast<int>(oneriler.size()) + (hesapSonucu.empty() ? 0 : 1); // Hesap sonucu varsa, panel bir satır daha fazla yer kaplar
    RECT istemciAlani;
    GetClientRect(m_pencere, &istemciAlani);
    int yukseklik = toplamSatirSayisi * OneriPaneliSatirYuksekligi + 8;
    ust += 6; // Adres çubuğunun tam dibine yapışık değil, hafifçe ayrık, kendi başına süzülen bir kart gibi görünsün diye küçük bir boşluk bırakılır
    RECT oneriAlani = { sol, ust, sol + max(200, genislik), min(istemciAlani.bottom, ust + yukseklik) };
    m_oneriDenetleyici->put_Bounds(oneriAlani);

    // Panelin GERÇEK alt penceresi, her gösterimde yeniden aranır (tutamak önceden saklanmaz): WebView2'nin aynı ana pencere altındaki toolbar/sekme/menü gibi diğer alt pencerelerinden,
    // konumu/boyutu o an put_Bounds ile verilen oneriAlani ile birebir eşleşen aday seçilerek ayırt edilir (menü panelindeki MenuPenceresiAra ile aynı yöntem)
    RECT oneriAlaniEkranKoordinati = oneriAlani;
    MapWindowPoints(m_pencere, nullptr, reinterpret_cast<POINT*>(&oneriAlaniEkranKoordinati), 2);
    MenuPenceresiAramaBaglami oneriPenceresiBaglami;
    oneriPenceresiBaglami.hedefAlaniEkranKoordinati = oneriAlaniEkranKoordinati;
    EnumChildWindows(m_pencere, MenuPenceresiAra, reinterpret_cast<LPARAM>(&oneriPenceresiBaglami));
    if (oneriPenceresiBaglami.bulunanPencere != nullptr)
    {
        SetWindowPos(oneriPenceresiBaglami.bulunanPencere, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); // Sonradan açılan sekmelerin WebView'i panelin üzerini örtmesin diye Z sırası öne alınır

        UINT dpi = GetDpiForWindow(m_pencere); // Panelin gerçek pencere sınırı da CSS'teki border-radius ile aynı yarıçapta kırpılır, yoksa köşelerde eski köşeli pencere görünmeye devam eder (bkz. MenuPaneliKoseleriniYuvarlaYap)
        int yaricap = MulDiv(10, static_cast<int>(dpi), 96) * 2;
        int paneliGenislik = oneriAlani.right - oneriAlani.left;
        int paneliYukseklik = oneriAlani.bottom - oneriAlani.top;
        HRGN bolge = CreateRoundRectRgn(0, 0, paneliGenislik + 1, paneliYukseklik + 1, yaricap, yaricap);
        SetWindowRgn(oneriPenceresiBaglami.bulunanPencere, bolge, TRUE);
    }

    std::wstring onerilerJson = L"["; // Öneri listesi, bir JSON dizisi olarak hazırlanır
    bool ilkGirdiYazildiMi = false;
    if (!hesapSonucu.empty()) // Hesap makinesi/birim çevirici sonucu varsa, her zaman listenin en başında gösterilir
    {
        onerilerJson += L"{\"calc\":true,\"title\":\"" + JsonIcinKacisla(hesapSonucu) + L"\"}";
        ilkGirdiYazildiMi = true;
    }
    for (size_t i = 0; i < oneriler.size(); i++)
    {
        if (ilkGirdiYazildiMi) { onerilerJson += L","; }
        ilkGirdiYazildiMi = true;
        onerilerJson += L"{\"address\":\"" + JsonIcinKacisla(oneriler[i].adres) + L"\",\"title\":\"" + JsonIcinKacisla(oneriler[i].baslik) + L"\"}";
    }
    onerilerJson += L"]";
    m_oneriWebView->PostWebMessageAsString((L"{\"type\":\"addresssuggestions\",\"query\":\"" + JsonIcinKacisla(sorgu) + L"\",\"entries\":" + onerilerJson + L"}").c_str());

    m_oneriPaneliAcikMi = true;
    m_oneriDenetleyici->put_IsVisible(TRUE);
}

// Bu fonksiyon, adres çubuğu öneri panelini kapatır (gizler)
void BrowserWindow::OneriPaneliniKapat()
{
    m_oneriPaneliAcikMi = false;
    m_oneriSecilenIndeks = -1; // Panel kapanınca, bir dahaki açılışta önceki klavye seçiminin kalıntısı kalmasın diye sıfırlanır
    if (m_oneriDenetleyici != nullptr)
    {
        m_oneriDenetleyici->put_IsVisible(FALSE);
    }
}

// Bu fonksiyon, adres çubuğunda yukarı/aşağı ok tuşuna basıldığında öneri panelindeki klavye seçimini bir satır ileri/geri taşır, panel kapalıysa veya hiç öneri yoksa hiçbir şey yapmaz
void BrowserWindow::AdresOnerisiGezin(const std::wstring& yon)
{
    if (!m_oneriPaneliAcikMi || m_oneriSeciliAdresler.empty()) { return; }

    int satirSayisi = static_cast<int>(m_oneriSeciliAdresler.size());
    if (yon == L"down")
    {
        m_oneriSecilenIndeks = min(m_oneriSecilenIndeks + 1, satirSayisi - 1); // Son satırdan öteye geçilmez
    }
    else if (yon == L"up")
    {
        m_oneriSecilenIndeks = max(m_oneriSecilenIndeks - 1, -1); // -1, hiçbir satırın seçili olmadığı, adres çubuğundaki orijinal yazının geçerli olduğu durumu temsil eder
    }

    if (m_oneriWebView != nullptr)
    {
        m_oneriWebView->PostWebMessageAsString((L"{\"type\":\"addresssuggestions_select\",\"index\":" + std::to_wstring(m_oneriSecilenIndeks) + L"}").c_str());
    }
}

// Bu fonksiyon, Enter'a basıldığında öneri panelinde klavyeyle seçili bir satır varsa ona gider ve true döner, hiçbir satır seçili değilse hiçbir şey yapmadan false döner
bool BrowserWindow::AdresOnerisiSeciliyiOnayla()
{
    if (!m_oneriPaneliAcikMi || m_oneriSecilenIndeks < 0 || m_oneriSecilenIndeks >= static_cast<int>(m_oneriSeciliAdresler.size())) { return false; }

    SekmeBilgisi* aktifSekme = AktifSekmeyiAl();
    if (aktifSekme == nullptr) { return false; }

    aktifSekme->webView->Navigate(m_oneriSeciliAdresler[m_oneriSecilenIndeks].c_str()); // Klavyeyle seçilen öneri, tıklamadaki ile aynı şekilde doğrudan adrese gider
    OneriPaneliniKapat();
    return true;
}

// Bu fonksiyon, WebView2'nin ham sertifika hatası durumunu, kullanıcıya gösterilecek sade bir Türkçe açıklamaya çevirir
std::wstring BrowserWindow::SertifikaHatasiAciklamasi(COREWEBVIEW2_WEB_ERROR_STATUS hataDurumu)
{
    switch (hataDurumu)
    {
        case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_COMMON_NAME_IS_INCORRECT: return L"Sertifika, bu sitenin adıyla eşleşmiyor.";
        case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_EXPIRED: return L"Sertifikanın süresi dolmuş.";
        case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_REVOKED: return L"Sertifika, yayınlayan kurum tarafından iptal edilmiş.";
        case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID: return L"Sertifika geçersiz veya güvenilir bir kurum tarafından imzalanmamış.";
        case COREWEBVIEW2_WEB_ERROR_STATUS_CLIENT_CERTIFICATE_CONTAINS_ERRORS: return L"Sertifikada hatalar var.";
        default: return L"Bağlantının güvenliğinde bir sorun tespit edildi.";
    }
}

// Bu fonksiyon, bir DATE (WebView2'nin sertifika tarihleri için kullandığı, 1899'un sonundan itibaren geçen gün sayısı cinsinden OLE otomasyon tarihi) değerini "GG.AA.YYYY" biçiminde bir metne çevirir
static std::wstring OleTarihiniMetneCevir(double oleTarihi)
{
    SYSTEMTIME sistemZamani = {};
    if (!VariantTimeToSystemTime(oleTarihi, &sistemZamani)) // Dönüştürme başarısız olursa (geçersiz bir tarih değeri gelirse)
    {
        return L"bilinmiyor";
    }
    wchar_t arabellek[32];
    swprintf_s(arabellek, L"%02d.%02d.%04d", sistemZamani.wDay, sistemZamani.wMonth, sistemZamani.wYear);
    return arabellek;
}

// Bu fonksiyon, gösterilecek bekleyen bir sertifika hatası varsa, sertifika panelini o hatanın bilgileriyle gösterir, hiç bekleyen hata yoksa paneli gizler
void BrowserWindow::SertifikaPanelindekiHatayiGuncelle()
{
    if (m_sertifikaDenetleyici == nullptr) // Denetleyici henüz hazır değilse hiçbir şey yapılamaz
    {
        return;
    }

    if (m_bekleyenSertifikaHatalari.empty()) // Gösterilecek bekleyen bir hata yoksa
    {
        m_sertifikaDenetleyici->put_IsVisible(FALSE); // Panel gizlenir
        return;
    }

    auto& [istekId, hata] = *m_bekleyenSertifikaHatalari.begin(); // En eski bekleyen hata alınır

    LPWSTR adresPtr = nullptr; // Hatanın oluştuğu adresi tutacak değişken
    hata.argumanlar->get_RequestUri(&adresPtr);
    std::wstring adres = adresPtr ? adresPtr : L"";
    if (adresPtr != nullptr) { CoTaskMemFree(adresPtr); }

    COREWEBVIEW2_WEB_ERROR_STATUS hataDurumu; // Hatanın türü
    hata.argumanlar->get_ErrorStatus(&hataDurumu);
    std::wstring aciklama = SertifikaHatasiAciklamasi(hataDurumu);

    std::wstring konu = L"bilinmiyor", yayinlayan = L"bilinmiyor", gecerliBaslangic = L"bilinmiyor", gecerliBitis = L"bilinmiyor";
    ComPtr<ICoreWebView2Certificate> sertifika; // Sertifikanın kendisi, temel bilgilerini (konu, yayınlayan, geçerlilik tarihleri) göstermek için okunur
    if (SUCCEEDED(hata.argumanlar->get_ServerCertificate(&sertifika)) && sertifika != nullptr)
    {
        LPWSTR konuPtr = nullptr;
        sertifika->get_Subject(&konuPtr);
        if (konuPtr != nullptr) { konu = konuPtr; CoTaskMemFree(konuPtr); }

        LPWSTR yayinlayanPtr = nullptr;
        sertifika->get_Issuer(&yayinlayanPtr);
        if (yayinlayanPtr != nullptr) { yayinlayan = yayinlayanPtr; CoTaskMemFree(yayinlayanPtr); }

        double gecerliBaslangicHam = 0.0, gecerliBitisHam = 0.0;
        sertifika->get_ValidFrom(&gecerliBaslangicHam);
        sertifika->get_ValidTo(&gecerliBitisHam);
        gecerliBaslangic = OleTarihiniMetneCevir(gecerliBaslangicHam);
        gecerliBitis = OleTarihiniMetneCevir(gecerliBitisHam);
    }

    std::wstring mesaj = L"{\"type\":\"certificateerror\",\"requestId\":" + std::to_wstring(istekId)
        + L",\"uri\":\"" + JsonIcinKacisla(adres) + L"\",\"description\":\"" + JsonIcinKacisla(aciklama) + L"\""
        + L",\"subject\":\"" + JsonIcinKacisla(konu) + L"\",\"issuer\":\"" + JsonIcinKacisla(yayinlayan) + L"\""
        + L",\"validFrom\":\"" + JsonIcinKacisla(gecerliBaslangic) + L"\",\"validTo\":\"" + JsonIcinKacisla(gecerliBitis) + L"\"}";
    m_sertifikaWebView->PostWebMessageAsString(mesaj.c_str()); // Hatanın bilgileri panele gönderilir

    BoyutlariGuncelle(); // Panelin konumu hesaplanır
    m_sertifikaDenetleyici->put_IsVisible(TRUE); // Panel görünür yapılır
    m_sertifikaDenetleyici->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC); // Odak panele taşınır
}

// Bu fonksiyon, sertifika hatası panelindeki "Geri Dön" veya "Yine de Git" butonuna tıklandığında çağrılır
void BrowserWindow::SertifikaHatasiYanitla(int istekId, bool devamEt)
{
    auto bulunan = m_bekleyenSertifikaHatalari.find(istekId); // Yanıtlanan hata, bekleyenler listesinde aranır
    if (bulunan == m_bekleyenSertifikaHatalari.end()) // Eğer bu kimlikte bir hata bulunamazsa (örneğin sekme bu arada kapatılıp hata zaten temizlenmişse)
    {
        return; // Yapılacak bir şey yoktur
    }

    BekleyenSertifikaHatasi hata = bulunan->second; // Hatanın bir kopyası alınır, map'ten silinmeden önce
    m_bekleyenSertifikaHatalari.erase(bulunan); // Hata, bekleyenler listesinden çıkarılır

    // Kullanıcı "Yine de Git" derse ALWAYS_ALLOW seçilir (WebView2 bu kararı bu sertifika için profil ömrü boyunca hatırlar, bir sonraki ziyarette bir daha sorulmaz)
    // "Geri Dön" derse CANCEL seçilir, gezinme iptal edilir ve sekme önceki (veya boş) sayfasında kalır
    hata.argumanlar->put_Action(devamEt ? COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_ALWAYS_ALLOW : COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL);
    hata.erteleme->Complete(); // Erteleme tamamlanır, gezinme karara göre devam eder veya iptal edilir

    SertifikaPanelindekiHatayiGuncelle(); // Sırada başka bir bekleyen hata varsa o gösterilir, yoksa panel gizlenir
}

// Bu fonksiyon, verilen dosya yolunun uzantısına göre, tarayıcıların "data:" adresinde beklediği MIME türünü tahmin eder
static std::wstring GorselMimeTuruTahminEt(const std::wstring& dosyaYolu)
{
    size_t nokta = dosyaYolu.find_last_of(L'.');
    std::wstring uzanti = (nokta == std::wstring::npos) ? L"" : dosyaYolu.substr(nokta + 1);
    for (wchar_t& harf : uzanti) { harf = static_cast<wchar_t>(towlower(harf)); }
    if (uzanti == L"png") { return L"image/png"; }
    if (uzanti == L"webp") { return L"image/webp"; }
    if (uzanti == L"gif") { return L"image/gif"; }
    return L"image/jpeg"; // jpg/jpeg ve tanınmayan her uzantı için makul bir varsayılan
}

// Bu fonksiyon, exe'nin bulunduğu klasörden başlayıp üst klasörlere doğru "resimler" adında bir klasör arar (hem geliştirme ortamındaki proje köküne hem de ileride çıkabilecek bir dağıtım klasörüne aynı anda uyması için), bulduğu ilk klasörün tam yolunu döndürür, hiçbir seviyede bulamazsa boş metin döndürür
static std::wstring ResimlerKlasorunuBul()
{
    wchar_t exeYolu[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exeYolu, MAX_PATH);
    std::wstring klasor = exeYolu;
    size_t sonTersBolu = klasor.find_last_of(L'\\');
    klasor = (sonTersBolu == std::wstring::npos) ? L"." : klasor.substr(0, sonTersBolu); // exe'nin bulunduğu klasörle başlanır

    for (int seviye = 0; seviye < 4; seviye++) // exe klasöründen başlayıp en fazla dört üst klasöre kadar aranır (x64\Debug gibi iki seviye derin çıktı klasörlerini de kapsar)
    {
        std::wstring aday = klasor + L"\\resimler";
        DWORD oznitelikler = GetFileAttributesW(aday.c_str());
        if (oznitelikler != INVALID_FILE_ATTRIBUTES && (oznitelikler & FILE_ATTRIBUTE_DIRECTORY)) { return aday; } // Bu seviyede "resimler" adında bir klasör bulunduysa hemen döndürülür

        size_t ustSeviye = klasor.find_last_of(L'\\');
        if (ustSeviye == std::wstring::npos) { break; } // Kök klasöre ulaşıldı, daha yukarı çıkılamaz
        klasor = klasor.substr(0, ustSeviye);
    }
    return L""; // Hiçbir seviyede bulunamadı
}

// Bu fonksiyon, resimler klasöründeki tüm desteklenen resim dosyalarının tam yollarını bir listeye toplar
static std::vector<std::wstring> ResimlerKlasorunuTara(const std::wstring& klasor)
{
    std::vector<std::wstring> resimYollari;
    WIN32_FIND_DATAW bulunan;
    HANDLE arama = FindFirstFileW((klasor + L"\\*").c_str(), &bulunan);
    if (arama != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (bulunan.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { continue; } // Alt klasörler atlanır, sadece doğrudan içindeki dosyalara bakılır
            std::wstring dosyaAdi = bulunan.cFileName;
            size_t nokta = dosyaAdi.find_last_of(L'.');
            if (nokta == std::wstring::npos) { continue; }
            std::wstring uzanti = dosyaAdi.substr(nokta + 1);
            for (wchar_t& harf : uzanti) { harf = static_cast<wchar_t>(towlower(harf)); }
            if (uzanti == L"jpg" || uzanti == L"jpeg" || uzanti == L"png" || uzanti == L"webp" || uzanti == L"gif")
            {
                resimYollari.push_back(klasor + L"\\" + dosyaAdi);
            }
        } while (FindNextFileW(arama, &bulunan));
        FindClose(arama);
    }
    return resimYollari;
}

// Bu fonksiyon, verilen kaynak resmi WIC (Windows Imaging Component) ile okuyup, en uzun kenarı azamiKenar pikseli aşmayacak şekilde küçültür ve JPEG olarak hedef dosyaya yazar
// Galeri karelerinde orijinal 2-6 MB'lık fotoğrafları tam çözünürlükte göstermek arayüzü ciddi şekilde kasıyordu, bu fonksiyon sayesinde her fotoğraf için küçük, tek seferlik bir önizleme dosyası diske yazılır ve bir daha hiç yeniden hesaplanmaz
static bool KucukOnizlemeOlustur(const std::wstring& kaynakYol, const std::wstring& hedefYol, UINT azamiKenar)
{
    ComPtr<IWICImagingFactory> fabrika;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fabrika)))) { return false; }

    ComPtr<IWICBitmapDecoder> cozucu;
    if (FAILED(fabrika->CreateDecoderFromFilename(kaynakYol.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &cozucu))) { return false; }

    ComPtr<IWICBitmapFrameDecode> kare;
    if (FAILED(cozucu->GetFrame(0, &kare))) { return false; }

    UINT genislik = 0, yukseklik = 0;
    kare->GetSize(&genislik, &yukseklik);
    if (genislik == 0 || yukseklik == 0) { return false; }

    double olcek = static_cast<double>(azamiKenar) / static_cast<double>(genislik > yukseklik ? genislik : yukseklik);
    UINT yeniGenislik = (olcek < 1.0) ? static_cast<UINT>(genislik * olcek) : genislik; // Resim azami kenardan zaten küçükse büyütülmez, olduğu gibi kopyalanır
    UINT yeniYukseklik = (olcek < 1.0) ? static_cast<UINT>(yukseklik * olcek) : yukseklik;
    if (yeniGenislik < 1) { yeniGenislik = 1; }
    if (yeniYukseklik < 1) { yeniYukseklik = 1; }

    ComPtr<IWICBitmapScaler> olcekleyici;
    if (FAILED(fabrika->CreateBitmapScaler(&olcekleyici))) { return false; }
    if (FAILED(olcekleyici->Initialize(kare.Get(), yeniGenislik, yeniYukseklik, WICBitmapInterpolationModeFant))) { return false; } // Fant filtresi, küçültmede en kaliteli (ama hâlâ hızlı) sonucu verir

    ComPtr<IWICFormatConverter> donusturucu;
    if (FAILED(fabrika->CreateFormatConverter(&donusturucu))) { return false; }
    if (FAILED(donusturucu->Initialize(olcekleyici.Get(), GUID_WICPixelFormat24bppBGR, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) { return false; } // JPEG kodlayıcısının beklediği düz 24bpp BGR formatına çevrilir

    ComPtr<IWICStream> akis;
    if (FAILED(fabrika->CreateStream(&akis))) { return false; }
    if (FAILED(akis->InitializeFromFilename(hedefYol.c_str(), GENERIC_WRITE))) { return false; }

    ComPtr<IWICBitmapEncoder> kodlayici;
    if (FAILED(fabrika->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, &kodlayici))) { return false; }
    if (FAILED(kodlayici->Initialize(akis.Get(), WICBitmapEncoderNoCache))) { return false; }

    ComPtr<IWICBitmapFrameEncode> kareKodlama;
    ComPtr<IPropertyBag2> ozellikCantasi;
    if (FAILED(kodlayici->CreateNewFrame(&kareKodlama, &ozellikCantasi))) { return false; }

    if (ozellikCantasi != nullptr) // JPEG kalitesi, önizleme için yeterli ama dosyayı küçük tutacak bir orta seviyeye ayarlanır
    {
        PROPBAG2 secenek = {};
        secenek.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
        VARIANT deger;
        VariantInit(&deger);
        deger.vt = VT_R4;
        deger.fltVal = 0.75f;
        ozellikCantasi->Write(1, &secenek, &deger);
    }

    if (FAILED(kareKodlama->Initialize(ozellikCantasi.Get()))) { return false; }
    if (FAILED(kareKodlama->SetSize(yeniGenislik, yeniYukseklik))) { return false; }
    WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
    if (FAILED(kareKodlama->SetPixelFormat(&format))) { return false; }
    if (FAILED(kareKodlama->WriteSource(donusturucu.Get(), nullptr))) { return false; }
    if (FAILED(kareKodlama->Commit())) { return false; }
    if (FAILED(kodlayici->Commit())) { return false; }

    return true;
}

// Bu fonksiyon, "resimler" klasöründeki tüm resimlerin dosya adlarını, Ayarlar sayfasındaki galerinin çizebilmesi için JSON olarak gönderir
// Resimlerin kendisi hiç Base64'e çevrilip JSON içine gömülmez (bu, onlarca megabaytlık tek bir mesaj demek olurdu); galeride gösterilen küçük önizlemeler de, orijinal fotoğraflar yerine ".thumbs" alt klasöründeki 128x128 kopyalardan (yoksa KucukOnizlemeOlustur ile ilk burada üretilip diske yazılır) servis edilir, böylece galeri her açıldığında megabaytlarca orijinal fotoğraf decode edilip arayüz kasmaz
void BrowserWindow::ArkaPlanGalerisiniGonder(int sekmeId)
{
    std::wstring klasor = ResimlerKlasorunuBul();
    std::vector<std::wstring> yollar = klasor.empty() ? std::vector<std::wstring>() : ResimlerKlasorunuTara(klasor);

    std::wstring onizlemeKlasoru = klasor.empty() ? L"" : (klasor + L"\\.thumbs"); // Küçük önizlemelerin tutulduğu, galeri taramasının asla göremediği (çünkü bir klasördür) gizli alt klasör
    if (!onizlemeKlasoru.empty()) { CreateDirectoryW(onizlemeKlasoru.c_str(), nullptr); }

    std::wstring listeMetni = L"[";
    for (size_t i = 0; i < yollar.size(); i++)
    {
        size_t ayracKonumu = yollar[i].find_last_of(L'\\');
        std::wstring dosyaAdi = (ayracKonumu == std::wstring::npos) ? yollar[i] : yollar[i].substr(ayracKonumu + 1);

        std::wstring onizlemeAdi = dosyaAdi + L".jpg"; // Önizleme her zaman JPEG olarak kaydedilir, orijinal uzantı ne olursa olsun (örn "foto.png.jpg"), çakışma riski olmadan tek bir biçime sabitlenir
        std::wstring onizlemeYolu = onizlemeKlasoru + L"\\" + onizlemeAdi;

        WIN32_FIND_DATAW onizlemeBilgisi;
        HANDLE onizlemeTanitici = FindFirstFileW(onizlemeYolu.c_str(), &onizlemeBilgisi);
        bool onizlemeVarMi = (onizlemeTanitici != INVALID_HANDLE_VALUE);
        if (onizlemeVarMi) { FindClose(onizlemeTanitici); }

        if (!onizlemeVarMi) // Bu fotoğrafın önizlemesi daha önce üretilmediyse, şimdi üretilip diske yazılır, bir sonraki açılışta bu adım hiç çalışmaz
        {
            onizlemeVarMi = KucukOnizlemeOlustur(yollar[i], onizlemeYolu, 128);
        }

        if (i > 0) { listeMetni += L","; }
        listeMetni += L"{\"name\":\"" + JsonIcinKacisla(dosyaAdi) + L"\",\"thumb\":\"" + JsonIcinKacisla(onizlemeVarMi ? (L".thumbs/" + onizlemeAdi) : dosyaAdi) + L"\"}"; // Önizleme üretilemediyse (örn bozuk dosya), son çare olarak orijinal gösterilir
    }
    listeMetni += L"]";

    SekmeyeMesajGonder(sekmeId, L"{\"type\":\"backgroundgallery\",\"images\":" + listeMetni + L"}");
}

// Bu fonksiyon, kullanıcı Ayarlar sayfasındaki galeriden bir resme tıkladığında, o resmi yeni sekme arka planı olarak kaydeder
void BrowserWindow::KlasordenArkaPlanSec(int sekmeId, const std::wstring& dosyaAdiHam)
{
    // Dosya adı, doğrudan bir klasör yoluyla birleştirileceği için, yol gezinme riskine karşı ters eğik çizgi, iki nokta gibi karakterler elenir, sadece gerçek bir dosya adında bulunabilecek karakterlere izin verilir
    std::wstring dosyaAdi;
    for (wchar_t karakter : dosyaAdiHam)
    {
        if (iswalnum(karakter) || karakter == L'_' || karakter == L'-' || karakter == L'.' || karakter == L' ') { dosyaAdi += karakter; }
    }
    if (dosyaAdi.empty()) { return; }

    std::wstring klasor = ResimlerKlasorunuBul();
    if (klasor.empty()) { return; }

    AyarVerisi ayarlar = Ayarlar::Oku();
    ayarlar.yeniSekmeArkaPlanYolu = klasor + L"\\" + dosyaAdi; // Galeriden seçilen resmin tam yolu, kullanıcının kendi dosyasını seçtiğindeki gibi aynı alana kaydedilir
    Ayarlar::Kaydet(ayarlar);
    AyarlarIcinVeriGonder(sekmeId, L"settings"); // Güncel ayarlar, Ayarlar sekmesine geri gönderilir
    YeniSekmeArkaPlaniniTumSekmelereGonder(); // Şu an açık olan hızlı erişim sekmeleri de yeni arka planı anında gösterir
}

// Bu fonksiyon, resimler klasöründeki resim dosyalarından birini "torba" (shuffle-bag) yöntemiyle seçer: m_gosterilmemisArkaPlanlar listesindeki tüm resimler, bir sonraki resim seçilmeden önce HEPSİ birer birer tüketilir, böylece klasörde beş resim varsa, altıncı seçime kadar hiçbiri ikinci kez tekrarlanmaz; torba boşalınca (veya klasördeki resim sayısı değiştiyse, örneğin kullanıcı yeni resim ekleyip sildiyse) klasör yeniden taranıp torba baştan doldurulur
std::wstring BrowserWindow::RastgeleKlasorResmiSec()
{
    std::wstring klasor = ResimlerKlasorunuBul();
    if (klasor.empty()) { return L""; } // Klasör hiçbir seviyede bulunamadı

    if (m_gosterilmemisArkaPlanlar.empty()) // Torba boşsa (program daha hiç resim göstermemiştir, ya da döngünün sonuna gelinmiştir), klasör yeniden taranıp torba baştan doldurulur
    {
        m_gosterilmemisArkaPlanlar = ResimlerKlasorunuTara(klasor);
    }
    if (m_gosterilmemisArkaPlanlar.empty()) { return L""; } // Klasör var ama içinde desteklenen bir resim yok

    static bool tohumlandiMi = false; // rand(), her program açılışında aynı sabit diziyi üretmesin diye bir kere, programın açılış anına göre tohumlanır
    if (!tohumlandiMi) { srand(static_cast<unsigned int>(GetTickCount64())); tohumlandiMi = true; }

    size_t secilenIndeks = static_cast<size_t>(rand()) % m_gosterilmemisArkaPlanlar.size(); // Torbadaki, henüz bu döngüde gösterilmemiş resimlerden biri rastgele seçilir
    std::wstring secilenYol = m_gosterilmemisArkaPlanlar[secilenIndeks];
    m_gosterilmemisArkaPlanlar.erase(m_gosterilmemisArkaPlanlar.begin() + secilenIndeks); // Seçilen resim torbadan çıkarılır, bu döngüde bir daha seçilemez
    return secilenYol;
}

// Bu fonksiyon, kullanıcının seçtiği özel arka plan resmini diskten okur, Base64 ile kodlayıp bir "data:" adresi olarak verilen sekmeye gönderir, kullanıcı elle bir resim seçmediyse "resimler" klasöründen rastgele bir resim denenir, o da yoksa boş bir adres gönderilir
void BrowserWindow::YeniSekmeArkaPlaniniGonder(int sekmeId)
{
    std::wstring yol = Ayarlar::Oku().yeniSekmeArkaPlanYolu; // Güncel ayarlardan, seçilmiş resmin yolu okunur
    if (yol.empty()) { yol = RastgeleKlasorResmiSec(); } // Elle seçilmiş bir resim yoksa, resimler klasöründen rastgele biri denenir
    if (yol.empty()) // Elle seçilmiş resim de, resimler klasörü de yoksa
    {
        SekmeyeMesajGonder(sekmeId, L"{\"type\":\"newtabbackground\",\"dataUrl\":null}");
        return;
    }

    std::ifstream dosya(yol, std::ios::binary | std::ios::ate); // Dosya, ham baytlar olarak ve sonundan (ate) açılır, böylece boyutu hemen bilinir
    if (!dosya) // Dosya artık yoksa (örneğin kullanıcı taşımış/silmişse)
    {
        SekmeyeMesajGonder(sekmeId, L"{\"type\":\"newtabbackground\",\"dataUrl\":null}");
        return;
    }

    std::streamsize boyut = dosya.tellg(); // Dosyanın toplam bayt boyutu okunur
    dosya.seekg(0, std::ios::beg); // Okuma konumu, dosyanın başına geri alınır

    std::vector<uint8_t> hamVeri(static_cast<size_t>(boyut)); // Dosyanın tamamını alacak büyüklükte bir arabellek ayrılır
    dosya.read(reinterpret_cast<char*>(hamVeri.data()), boyut); // Dosyanın tüm içeriği okunur

    std::wstring dataUrl = L"data:" + GorselMimeTuruTahminEt(yol) + L";base64," + Base64Kodla(hamVeri); // Ham baytlar, tarayıcıların doğrudan bir <img> kaynağı olarak kullanabileceği bir "data:" adresine çevrilir
    SekmeyeMesajGonder(sekmeId, L"{\"type\":\"newtabbackground\",\"dataUrl\":\"" + dataUrl + L"\"}");
}

// Bu fonksiyon, açık olan tüm hızlı erişim sekmelerine, güncel özel arka plan resmini yeniden gönderir
void BrowserWindow::YeniSekmeArkaPlaniniTumSekmelereGonder()
{
    for (const std::unique_ptr<SekmeBilgisi>& sekme : m_sekmeler) // Açık olan her sekme sırayla kontrol edilir
    {
        if (sekme->dahiliSayfaMi && sekme->webView != nullptr) // Sadece dahili sekmelere gönderilir, hızlı erişim sayfası olmayan dahili sayfalar (Ayarlar gibi) bu mesajı zaten görmezden gelir
        {
            YeniSekmeArkaPlaniniGonder(sekme->id);
        }
    }
}

// Bu fonksiyon, bir sayfanın alan adının (host), kayıtlı bir şifrenin "site" alanıyla aynı siteye ait olup olmadığını, "www." önekini göz ardı ederek karşılaştırır
static bool AyniSiteMi(const std::wstring& host, const std::wstring& kayitliSite)
{
    auto kucultVeWwwAt = [](std::wstring metin) -> std::wstring
    {
        for (wchar_t& harf : metin) { harf = static_cast<wchar_t>(towlower(harf)); } // Küçük harfe çevrilir, büyük/küçük harf farkı önemsiz olsun diye
        if (metin.rfind(L"www.", 0) == 0) { metin = metin.substr(4); } // Baştaki "www." öneki varsa atılır
        return metin;
    };
    std::wstring a = kucultVeWwwAt(host);
    std::wstring b = kucultVeWwwAt(kayitliSite);
    return !a.empty() && !b.empty() && a == b; // Tam eşleşme aranır, boş bir değer asla eşleşmez
}

// Bu fonksiyon, bir sayfadaki şifre alanına tıklanınca gelen (ayrıcalıksız) "autofill_check" mesajını işler
// gercekKaynakAdresi, WebView2'nin kendi bildirdiği GERÇEK adrestir, sayfanın mesaj içine koyduğu bir veri değildir, bu yüzden güvenle kullanılabilir
void BrowserWindow::OtomatikDoldurmaIsteginiIsle(int sekmeId, const std::wstring& gercekKaynakAdresi, const std::wstring& mesaj)
{
    if (mesaj.rfind(L"autofill_check|", 0) != 0) // Bu dar işleyici, SADECE bu tek komutu tanır, başka her şey (ayrıcalıklı olsun olmasın) sessizce yok sayılır
    {
        return;
    }
    if (m_doldurmaDenetleyici == nullptr) // Panel henüz hazır değilse gösterilecek bir şey yoktur
    {
        return;
    }

    std::wstring host = HostCikar(gercekKaynakAdresi); // Mesajın GERÇEKTEN geldiği site, güvenilir kaynaktan çıkarılır

    std::vector<std::wstring> eslesenKullaniciAdlari; // Bu siteye ait, kayıtlı tüm kullanıcı adları (şifrelerin kendisi burada ASLA okunmaz/çözülmez)
    for (const SifreKaydi& kayit : SifreKasasi::SifreleriOku())
    {
        if (AyniSiteMi(host, kayit.site)) { eslesenKullaniciAdlari.push_back(kayit.kullaniciAdi); }
    }

    if (eslesenKullaniciAdlari.empty()) // Bu site için kayıtlı hiçbir hesap yoksa, panelin gösterilecek bir şeyi yoktur
    {
        DoldurmaPaneliniKapat();
        return;
    }

    std::wstring koordJson = mesaj.substr(15); // "autofill_check|" önekinden sonraki, alanın ekran koordinatlarını içeren küçük JSON nesnesi
    int ustKoordinat = _wtoi(JsonAlaniBul(koordJson, L"top").c_str());
    int solKoordinat = _wtoi(JsonAlaniBul(koordJson, L"left").c_str());

    m_doldurmaSekmeId = sekmeId; // Panel, artık bu sekmeye hizmet ediyor
    m_doldurmaHost = host; // Panel, artık bu siteye ait önerileri gösteriyor, "seçim yap" bu host'a göre doğrulanacak

    RECT istemciAlani; // Pencerenin iç alanı, panelin pencere dışına taşmaması için sınır olarak kullanılır
    GetClientRect(m_pencere, &istemciAlani);
    int yukseklik = static_cast<int>(eslesenKullaniciAdlari.size()) * DoldurmaPaneliYuksekligiSatirBasina + 8;
    int gercekSol = min(solKoordinat, max(0, istemciAlani.right - DoldurmaPaneliGenisligi)); // Panel, sağ kenardan taşmayacak şekilde konumlanır
    int gercekUst = ToplamToolbarYuksekligi() + ustKoordinat; // Alanın sayfadaki konumu, içerik alanının native penceredeki başlangıcına eklenir
    RECT doldurmaAlani = { gercekSol, gercekUst, gercekSol + DoldurmaPaneliGenisligi, min(istemciAlani.bottom, gercekUst + yukseklik) };
    m_doldurmaDenetleyici->put_Bounds(doldurmaAlani);

    std::wstring kullanicilarJson = L"["; // Öneri listesi, bir JSON dizisi olarak hazırlanır
    for (size_t i = 0; i < eslesenKullaniciAdlari.size(); i++)
    {
        if (i > 0) { kullanicilarJson += L","; }
        kullanicilarJson += L"\"" + JsonIcinKacisla(eslesenKullaniciAdlari[i]) + L"\"";
    }
    kullanicilarJson += L"]";
    m_doldurmaWebView->PostWebMessageAsString((L"{\"type\":\"autofillsuggestions\",\"usernames\":" + kullanicilarJson + L"}").c_str());

    m_doldurmaDenetleyici->put_IsVisible(TRUE); // Panel görünür yapılır
}

// Bu fonksiyon, otomatik doldurma panelinde bir kullanıcı adı seçildiğinde çağrılır
void BrowserWindow::OtomatikDoldurmaSecimiYap(const std::wstring& kullaniciAdi)
{
    std::wstring host = m_doldurmaHost; // Seçimin uygulanacağı site, panel açılırken native tarafın kendi belirlediği host'tur, panelden gelen bir veri DEĞİLDİR
    int sekmeId = m_doldurmaSekmeId;
    DoldurmaPaneliniKapat(); // Panel, seçim yapılır yapılmaz kapatılır

    bool dogrulandi = Guvenlik::WindowsSifresiniKilitlenmeSuresiyleDogrula(m_pencere, Ayarlar::Oku().sifreKasasiKilitlenmeDakikasi); // Şifre sayfaya yazılmadan önce, reveal_password ile aynı Windows kimlik doğrulaması istenir
    if (!dogrulandi)
    {
        return;
    }

    for (const SifreKaydi& kayit : SifreKasasi::SifreleriOku()) // Seçilen kullanıcı adına ve doğrulanmış host'a ait kayıt aranır
    {
        if (AyniSiteMi(host, kayit.site) && kayit.kullaniciAdi == kullaniciAdi)
        {
            std::wstring duzSifre = SifreKasasi::SifreyiCoz(kayit.sifreliVeri); // Şifre, DPAPI ile çözülür
            std::wstring mesaj = L"{\"type\":\"autofill_fill\",\"username\":\"" + JsonIcinKacisla(kullaniciAdi) + L"\",\"password\":\"" + JsonIcinKacisla(duzSifre) + L"\"}";
            SekmeyeMesajGonder(sekmeId, mesaj); // Düz metin şifre, SADECE isteği yapan o sekmeye, bir kere kullanılacak şekilde gönderilir, hiçbir yerde saklanmaz
            return;
        }
    }
}

// Bu fonksiyon, otomatik doldurma panelini kapatır (gizler)
void BrowserWindow::DoldurmaPaneliniKapat()
{
    if (m_doldurmaDenetleyici != nullptr)
    {
        m_doldurmaDenetleyici->put_IsVisible(FALSE);
    }
    m_doldurmaSekmeId = -1;
    m_doldurmaHost.clear();
}

// Bu fonksiyon, verilen sekmeye, native taraftan bir metin mesajı gönderir
void BrowserWindow::SekmeyeMesajGonder(int sekmeId, const std::wstring& mesaj)
{
    SekmeBilgisi* sekme = SekmeBul(sekmeId); // Mesaj gönderilecek sekmenin bilgilerine erişilir
    if (sekme != nullptr && sekme->webView != nullptr) // Sekme hâlâ açıksa ve WebView2'si hazırsa
    {
        sekme->webView->PostWebMessageAsString(mesaj.c_str()); // Verilen metin, bu sekmedeki JavaScript koduna düz bir metin mesajı olarak gönderilir
    }
}

// Bu fonksiyon, kullanıcının seçtiği arama motoruna göre, verilen (zaten adres için kodlanmış) arama metnini tam bir arama adresine çevirir
std::wstring BrowserWindow::AramaAdresiOlustur(const std::wstring& aramaMotoru, const std::wstring& kodlanmisSorgu)
{
    if (aramaMotoru == L"bing") // Eğer kullanıcı Bing'i seçtiyse
    {
        return L"https://www.bing.com/search?q=" + kodlanmisSorgu; // Bing arama adresi oluşturulur
    }
    if (aramaMotoru == L"duckduckgo") // Eğer kullanıcı DuckDuckGo'yu seçtiyse
    {
        return L"https://duckduckgo.com/?q=" + kodlanmisSorgu; // DuckDuckGo arama adresi oluşturulur
    }
    return L"https://www.google.com/search?q=" + kodlanmisSorgu; // Diğer tüm durumlarda (varsayılan olarak) Google arama adresi oluşturulur
}

// Bu fonksiyon, adres çubuğuna yazılan metnin bir web adresi mi yoksa bir arama sorgusu mu olduğuna karar verir, sonra aktif sekmeyi o adrese götürür
void BrowserWindow::AkilliAdreseGit(const std::wstring& girilenMetin)
{
    SekmeBilgisi* aktifSekme = AktifSekmeyiAl(); // Gidilecek sekme, aktif sekme olarak alınır
    if (aktifSekme == nullptr) // Eğer hiç aktif sekme yoksa
    {
        return; // Gidilecek bir yer olmadığı için fonksiyondan çıkılır
    }

    std::wstring metin = girilenMetin; // Üzerinde değişiklik yapabilmek için metnin bir kopyası alınır

    bool httpIleBasliyor = (metin.rfind(L"http://", 0) == 0) || (metin.rfind(L"https://", 0) == 0); // Metnin zaten bir web adresi gibi başlayıp başlamadığı kontrol edilir
    bool bosluklaIceriyor = (metin.find(L' ') != std::wstring::npos); // Metnin içinde bir boşluk olup olmadığı kontrol edilir
    bool noktaIceriyor = (metin.find(L'.') != std::wstring::npos); // Metnin içinde bir nokta olup olmadığı kontrol edilir

    if (httpIleBasliyor) // Eğer metin zaten tam bir web adresi gibi başlıyorsa
    {
        aktifSekme->webView->Navigate(metin.c_str()); // Metin olduğu gibi bir web adresi olarak açılır
    }
    else if (!bosluklaIceriyor && noktaIceriyor) // Eğer metin boşluk içermiyorsa ve bir nokta içeriyorsa, bu bir alan adı gibi görünür
    {
        aktifSekme->webView->Navigate((L"https://" + metin).c_str()); // Başına https:// eklenerek bir web adresi olarak açılır
    }
    else // Diğer tüm durumlarda, metin bir arama sorgusu olarak kabul edilir
    {
        std::wstring aramaAdresi = AramaAdresiOlustur(Ayarlar::Oku().aramaMotoru, MetniAdresIcinKodla(metin)); // Kullanıcının seçtiği arama motoruna göre arama adresi oluşturulur
        aktifSekme->webView->Navigate(aramaAdresi.c_str()); // Oluşan arama adresi açılır
    }
}

// Bu fonksiyon, aktif sekmedeki güncel adresi, geri/ileri gidilip gidilemeyeceği bilgisini, ve yer imi durumunu toplayıp toolbar arayüzüne gönderir
void BrowserWindow::GezinmeDurumunuGuncelleVeGonder()
{
    SekmeBilgisi* aktifSekme = AktifSekmeyiAl(); // Durumu gönderilecek sekme, aktif sekme olarak alınır
    if (aktifSekme == nullptr) // Eğer hiç aktif sekme yoksa
    {
        return; // Gönderilecek bir durum olmadığı için fonksiyondan çıkılır
    }

    LPWSTR guncelAdresHam = nullptr; // Güncel sayfa adresini tutacak değişken
    aktifSekme->webView->get_Source(&guncelAdresHam); // Aktif sekmenin şu anda gösterdiği sayfanın adresi okunur
    std::wstring guncelAdres = guncelAdresHam ? guncelAdresHam : L""; // Adres, güvenli bir metin değişkenine aktarılır
    if (guncelAdresHam != nullptr) { CoTaskMemFree(guncelAdresHam); } // Ayrılan bellek serbest bırakılır

    BOOL geriGidilebilir = FALSE; // Geri gidilip gidilemeyeceğini tutacak değişken
    aktifSekme->webView->get_CanGoBack(&geriGidilebilir); // Aktif sekmeden geri gidilip gidilemeyeceği bilgisi okunur

    BOOL ileriGidilebilir = FALSE; // İleri gidilip gidilemeyeceğini tutacak değişken
    aktifSekme->webView->get_CanGoForward(&ileriGidilebilir); // Aktif sekmeden ileri gidilip gidilemeyeceği bilgisi okunur

    bool yerImindeMi = Depolama::YerImindeMi(guncelAdres); // Güncel sayfanın yer imlerinde olup olmadığı kontrol edilir

    double yakinlastirmaOrani = 1.0; // Aktif sekmenin güncel yakınlaştırma oranını tutacak değişken, denetleyici henüz hazır değilse varsayılan %100 kullanılır
    if (aktifSekme->denetleyici != nullptr) // Denetleyici hazırsa
    {
        aktifSekme->denetleyici->get_ZoomFactor(&yakinlastirmaOrani); // Güncel oran okunur
    }
    int yakinlastirmaYuzdesi = static_cast<int>(yakinlastirmaOrani * 100.0 + 0.5); // Oran, hamburger menüsünde gösterilecek tam sayı bir yüzdeye çevrilir

    bool guvenliMi = (guncelAdres.rfind(L"https://", 0) == 0); // Adresin https ile başlayıp başlamadığı, kilit ikonunun durumu için kontrol edilir, dahili tarayici.local sayfaları da https şemasını kullandığı için zaten güvenli sayılır

    // Tüm bu bilgiler, toolbar arayüzünün JSON.parse ile okuyabileceği bir JSON nesnesi metni haline getirilir
    std::wstring durumMesaji = L"{\"type\":\"state\",\"url\":\"" + JsonIcinKacisla(guncelAdres) + L"\",\"canGoBack\":" + (geriGidilebilir ? L"true" : L"false") + L",\"canGoForward\":" + (ileriGidilebilir ? L"true" : L"false") + L",\"bookmarked\":" + (yerImindeMi ? L"true" : L"false") + L",\"secure\":" + (guvenliMi ? L"true" : L"false") + L",\"zoomPercent\":" + std::to_wstring(yakinlastirmaYuzdesi) + L"}";

    ToolbaraMesajGonder(durumMesaji); // Hazırlanan durum mesajı toolbar arayüzüne gönderilir

    if (m_hamburgerMenusuAcikMi) // Menü açıkken yakınlaştırma değiştiyse (örneğin menüdeki +/- butonlarıyla), panel de anında güncellenir
    {
        MenuyeMesajGonder(L"{\"type\":\"zoom\",\"zoomPercent\":" + std::to_wstring(yakinlastirmaYuzdesi) + L"}");
    }

    if (m_sitePaneliAcikMi) // Site bilgisi paneli açıkken aktif sekme değiştiyse veya içindeki sayfa gezindiyse, panel güncel siteye göre yeniden hesaplanır
    {
        m_sitePaneliKaynagi = guncelAdres; // Panelin ait olduğu site, yeni güncel adrese güncellenir
        SiteBilgisiPaneliVeriGonder(); // Panelin içeriği (güvenlik durumu, izin listesi) yeni siteye göre yeniden gönderilir
    }
}

// Bu fonksiyon, açık olan tüm sekmelerin kimlik numarası, başlığı, yükleniyor durumu ve uykuda olup olmadığı bilgisini, aktif sekmenin kimliğiyle birlikte toolbar arayüzüne gönderir
void BrowserWindow::SekmeListesiniGonder()
{
    std::wstring sekmelerMetni = L"["; // Sekme listesinin JSON dizisi, açılış köşeli parantezi ile başlanır
    for (size_t i = 0; i < m_sekmeler.size(); i++) // Açık olan her sekme sırayla gezilir
    {
        if (m_sekmeler[i]->dahiliSayfaMi) // Dahili sayfalar (örneğin Ayarlar) sekme şeridinde normal bir sekme olarak görünse de, bu kontrol gelecekte farklı bir simge gösterebilmek için saklanmıştır
        {
            // Şimdilik dahili sekmeler de normal sekmeler gibi listelenir
        }

        if (i > 0) // İlk sekmeden sonraki her sekmeden önce
        {
            sekmelerMetni += L","; // Sekmeleri birbirinden ayırmak için virgül eklenir
        }
        // Her sekme, kimlik numarası, başlık, yükleniyor durumu, uykuda olup olmadığı ve gizli olup olmadığı bilgisini içeren bir JSON nesnesi olarak yazılır
        sekmelerMetni += L"{\"id\":" + std::to_wstring(m_sekmeler[i]->id) + L",\"title\":\"" + JsonIcinKacisla(m_sekmeler[i]->baslik) + L"\",\"loading\":" + (m_sekmeler[i]->yukleniyor ? L"true" : L"false") + L",\"suspended\":" + (m_sekmeler[i]->askidaMi ? L"true" : L"false") + L",\"private\":" + (m_sekmeler[i]->gizliMi ? L"true" : L"false") + L",\"pinned\":" + (m_sekmeler[i]->sabitliMi ? L"true" : L"false") + L",\"muted\":" + (m_sekmeler[i]->sessizeAlindiMi ? L"true" : L"false") + L",\"playingAudio\":" + (m_sekmeler[i]->sesCaliyorMu ? L"true" : L"false") + L",\"groupId\":" + std::to_wstring(m_sekmeler[i]->grupId) + L",\"favicon\":\"" + m_sekmeler[i]->favicon + L"\"}"; // Favicon zaten base64 bir veri adresi olduğu için JSON kaçışına ihtiyaç duymaz
    }
    sekmelerMetni += L"]"; // JSON dizisinin kapanış köşeli parantezi eklenir

    std::wstring gruplarMetni = L"["; // Sekme gruplarının JSON dizisi
    for (size_t i = 0; i < m_sekmeGruplari.size(); i++)
    {
        if (i > 0) { gruplarMetni += L","; }
        gruplarMetni += L"{\"id\":" + std::to_wstring(m_sekmeGruplari[i].id) + L",\"name\":\"" + JsonIcinKacisla(m_sekmeGruplari[i].isim) + L"\",\"color\":\"" + JsonIcinKacisla(m_sekmeGruplari[i].renk) + L"\"}";
    }
    gruplarMetni += L"]";

    std::wstring mesaj = L"{\"type\":\"tabs\",\"activeId\":" + std::to_wstring(m_secimSekmeId) + L",\"tabs\":" + sekmelerMetni + L",\"groups\":" + gruplarMetni + L"}"; // Şeritte seçili görünmesi gereken sekmenin kimliği, sekme listesi ve grup listesi, tek bir JSON nesnesinde birleştirilir, denetleyicisi henüz hazırlanıyor olan bir sekme de burada seçili görünebilir
    ToolbaraMesajGonder(mesaj); // Hazırlanan sekme listesi mesajı toolbar arayüzüne gönderilir

    if (Ayarlar::Oku().sonOturumuGeriYukle) // Kullanıcı bu özelliği açtıysa, sekme listesi her değiştiğinde oturum dosyası da güncel tutulur, böylece bir çökme anında bile en güncel sekme listesi diskte olur
    {
        std::vector<SayfaKaydi> acikSekmeler;
        for (const std::unique_ptr<SekmeBilgisi>& sekme : m_sekmeler)
        {
            if (sekme->dahiliSayfaMi || sekme->gizliMi) { continue; } // Dahili sayfalar (Ayarlar gibi) ve gizli sekmeler oturuma kaydedilmez
            LPWSTR adresPtr = nullptr;
            if (sekme->webView != nullptr) { sekme->webView->get_Source(&adresPtr); }
            std::wstring adres = adresPtr ? adresPtr : L"";
            if (adresPtr != nullptr) { CoTaskMemFree(adresPtr); }
            if (!adres.empty()) { acikSekmeler.push_back({ adres, sekme->baslik, L"", L"" }); }
        }
        Depolama::OturumuKaydet(acikSekmeler);
    }
}

// Bu fonksiyon, verilen sekmede, verilen hata koduna sahip, kullanıcıya gösterilecek basit bir hata sayfası açar, ve bu hatayı günlük dosyasına kaydeder
void BrowserWindow::HataSayfasiGoster(int sekmeId, const std::wstring& kod, const std::wstring& basarisizAdres)
{
    std::wstring aciklama = Gunluk::KoduAciklamayaCevir(kod); // Hata kodunun kısa, kullanıcının anlayacağı açıklaması bulunur
    Gunluk::HataKaydet(kod, L"Adres: " + basarisizAdres); // Hata, kodu ve ilgili adresle birlikte günlük dosyasına kaydedilir

    SekmeBilgisi* sekme = SekmeBul(sekmeId); // Hata sayfası gösterilecek sekmenin bilgilerine erişilir
    if (sekme == nullptr) // Eğer sekme artık açık değilse
    {
        return; // Gösterilecek bir yer kalmadığı için fonksiyondan çıkılır
    }

    std::wstring yenidenDenemeButon = L""; // Yeniden deneme bağlantısı, başlangıçta boş bırakılır
    if (!basarisizAdres.empty()) // Eğer başarısız olan bir adres varsa
    {
        yenidenDenemeButon = L"<p><a href=\"" + basarisizAdres + L"\" style=\"color:#7dd3fc;\">Yeniden Dene</a></p>"; // Bu adrese tıklanarak tekrar gidilebilecek bir bağlantı hazırlanır
    }

    std::wstring htmlIcerik =
        L"<html><head><title>Hata - " + kod + L"</title></head><body style=\"font-family:Segoe UI, Arial, sans-serif; background-color:#1a1b22; color:#e8e8ea; padding:48px;\">"
        L"<h2>Bu sayfa açılamadı</h2>"
        L"<p style=\"color:#9a9ba5;\">Hata kodu: <b>" + kod + L"</b></p>"
        L"<p>" + aciklama + L"</p>"
        + yenidenDenemeButon +
        L"</body></html>";

    std::wstring dataAdresi = L"data:text/html;charset=utf-8," + MetniAdresIcinKodla(htmlIcerik); // Hazırlanan HTML, bir data: adresine dönüştürülür
    sekme->webView->Navigate(dataAdresi.c_str()); // Sekme, bu hata sayfasını gösteren adrese yönlendirilir
}

// Bu fonksiyon, WebView2'nin kendi ağ hatası durumunu, bizim kendi kategori ve numara içeren hata koduna çevirir
std::wstring BrowserWindow::AgHatasiniKoduCevir(COREWEBVIEW2_WEB_ERROR_STATUS hataDurumu)
{
    switch (hataDurumu) // Gelen ham hata durumuna göre, uygun kod seçilir
    {
        case COREWEBVIEW2_WEB_ERROR_STATUS_HOST_NAME_NOT_RESOLVED: return L"NET-1001"; // Adres (alan adı) çözümlenemedi
        case COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_RESET: // Bağlantı sıfırlandı
        case COREWEBVIEW2_WEB_ERROR_STATUS_CANNOT_CONNECT: return L"NET-1002"; // Bağlantı kurulamadı
        case COREWEBVIEW2_WEB_ERROR_STATUS_TIMEOUT: return L"NET-1003"; // Bağlantı zaman aşımına uğradı
        case COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED: return L"NET-1004"; // İnternet bağlantısı yok
        case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_COMMON_NAME_IS_INCORRECT: // Sertifika adı uyuşmuyor
        case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_EXPIRED: // Sertifikanın süresi dolmuş
        case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID: return L"NET-1005"; // Sertifika ile ilgili başka bir sorun var
        default: return L"NET-1099"; // Yukarıdaki kategorilerin hiçbirine uymayan, bilinmeyen bir ağ hatası
    }
}

// Bu fonksiyon, Ayarlar sekmesine, güncel ayarları, geçmişi, yer imlerini, şifreleri, hata günlüğünü veya Hakkında bilgisini JSON olarak gönderir
void BrowserWindow::AyarlarIcinVeriGonder(int sekmeId, const std::wstring& komut)
{
    if (komut == L"settings") // Güncel ayarlar isteniyorsa
    {
        AyarVerisi a = Ayarlar::Oku(); // Ayarlar diskten okunur
        std::wstring kisayollarJson = L"{\"newtab\":\"" + JsonIcinKacisla(std::wstring(1, a.kisayolYeniSekme)) + L"\",\"closetab\":\"" + JsonIcinKacisla(std::wstring(1, a.kisayolSekmeKapat)) + L"\",\"duplicate\":\"" + JsonIcinKacisla(std::wstring(1, a.kisayolCogalt)) + L"\",\"print\":\"" + JsonIcinKacisla(std::wstring(1, a.kisayolYazdir)) + L"\",\"find\":\"" + JsonIcinKacisla(std::wstring(1, a.kisayolSayfadaBul)) + L"\",\"reload\":\"" + JsonIcinKacisla(std::wstring(1, a.kisayolYenile)) + L"\",\"address\":\"" + JsonIcinKacisla(std::wstring(1, a.kisayolAdres)) + L"\"}"; // Özelleştirilebilir kısayolların güncel harfleri, tek bir nesnede toplanır, kaçışlanır çünkü kullanıcı teorik olarak " veya \ gibi bir karakter de girebilir
        std::wstring mesaj = L"{\"type\":\"settings\",\"searchEngine\":\"" + JsonIcinKacisla(a.aramaMotoru) + L"\",\"homepage\":\"" + JsonIcinKacisla(a.anaSayfa) + L"\",\"lockMinutes\":" + std::to_wstring(a.sifreKasasiKilitlenmeDakikasi) + L",\"sleepThresholdMinutes\":" + std::to_wstring(a.uykuSekmeEsigiDakika) + L",\"backgroundPath\":\"" + JsonIcinKacisla(a.yeniSekmeArkaPlanYolu) + L"\",\"shortcuts\":" + kisayollarJson + L",\"theme\":" + TemaNesnesiJsonYap(a)
            + L",\"exitClean\":" + (a.cikistaTemizle ? L"true" : L"false") + L",\"exitCleanCategories\":\"" + JsonIcinKacisla(a.cikistaTemizleKategoriler) + L"\",\"exemptSites\":\"" + JsonIcinKacisla(a.haricTutulanSiteler) + L"\",\"askDownloadLocation\":" + (a.indirmeKonumunuHerSeferSor ? L"true" : L"false") + L",\"energySaver\":" + (a.enerjiTasarrufuModu ? L"true" : L"false") + L",\"restoreSession\":" + (a.sonOturumuGeriYukle ? L"true" : L"false") + L",\"breachCheckEnabled\":" + (a.sizintiKontroluAcikMi ? L"true" : L"false") + L",\"adBlockEnabled\":" + (a.reklamEngellemeAcikMi ? L"true" : L"false") + L",\"trackerLevel\":\"" + JsonIcinKacisla(a.izleyiciEngellemeSeviyesi) + L"\",\"safeBrowsingEnabled\":" + (a.guvenliGezinmeAcikMi ? L"true" : L"false") + L",\"language\":\"" + JsonIcinKacisla(a.tarayiciDili) + L"\""
            + L"}"; // Tüm ayarlar tek bir JSON nesnesinde toplanır
        SekmeyeMesajGonder(sekmeId, mesaj); // Hazırlanan mesaj gönderilir
    }
    else if (komut == L"history") // Geçmiş listesi isteniyorsa
    {
        std::vector<SayfaKaydi> liste = Depolama::GecmisiOku(); // Geçmiş diskten okunur
        std::wstring mesaj = L"{\"type\":\"history\",\"entries\":" + SayfaKayitlariniJsonYap(liste) + L"}"; // Liste, bir JSON dizisi olarak sarmalanır
        SekmeyeMesajGonder(sekmeId, mesaj); // Hazırlanan mesaj gönderilir
    }
    else if (komut == L"bookmarks") // Yer imi listesi isteniyorsa
    {
        std::vector<SayfaKaydi> liste = Depolama::YerImleriniOku(); // Yer imleri diskten okunur
        std::wstring mesaj = L"{\"type\":\"bookmarks\",\"entries\":" + SayfaKayitlariniJsonYap(liste) + L"}"; // Liste, bir JSON dizisi olarak sarmalanır
        SekmeyeMesajGonder(sekmeId, mesaj); // Hazırlanan mesaj gönderilir
    }
    else if (komut == L"cards") // Kayıtlı kartların listesi isteniyorsa (kart numaralarını içermez)
    {
        std::vector<KayitliKart> liste = OtomatikDoldurmaKasasi::KartlariOku();
        std::wstring girdilerMetni = L"[";
        for (size_t i = 0; i < liste.size(); i++)
        {
            if (i > 0) { girdilerMetni += L","; }
            girdilerMetni += L"{\"kartSahibi\":\"" + JsonIcinKacisla(liste[i].kartSahibi) + L"\",\"sonKullanma\":\"" + JsonIcinKacisla(liste[i].sonKullanma) + L"\",\"eklenmeZamani\":\"" + JsonIcinKacisla(liste[i].eklenmeZamani) + L"\"}";
        }
        girdilerMetni += L"]";
        SekmeyeMesajGonder(sekmeId, L"{\"type\":\"cards\",\"entries\":" + girdilerMetni + L"}");
    }
    else if (komut == L"userscripts") // Kayıtlı kullanıcı betiklerinin listesi isteniyorsa
    {
        std::vector<KullaniciBetigi> liste = BetikKasasi::BetikleriOku();
        std::wstring girdilerMetni = L"[";
        for (size_t i = 0; i < liste.size(); i++)
        {
            if (i > 0) { girdilerMetni += L","; }
            girdilerMetni += L"{\"id\":\"" + JsonIcinKacisla(liste[i].id) + L"\",\"ad\":\"" + JsonIcinKacisla(liste[i].ad) + L"\",\"desen\":\"" + JsonIcinKacisla(liste[i].desen) + L"\",\"kod\":\"" + JsonIcinKacisla(liste[i].kod) + L"\",\"etkinMi\":" + (liste[i].etkinMi ? L"true" : L"false") + L"}";
        }
        girdilerMetni += L"]";
        SekmeyeMesajGonder(sekmeId, L"{\"type\":\"userscripts\",\"entries\":" + girdilerMetni + L"}");
    }
    else if (komut == L"readinglist") // Okuma listesi isteniyorsa
    {
        std::vector<SayfaKaydi> liste = Depolama::OkumaListesiniOku(); // Okuma listesi diskten okunur
        std::wstring mesaj = L"{\"type\":\"readinglist\",\"entries\":" + SayfaKayitlariniJsonYap(liste) + L"}"; // Liste, bir JSON dizisi olarak sarmalanır
        SekmeyeMesajGonder(sekmeId, mesaj); // Hazırlanan mesaj gönderilir
    }
    else if (komut == L"errorlog") // Hata günlüğü isteniyorsa
    {
        std::vector<HataGirdisi> liste = Gunluk::SonHatalariOku(200); // En güncel iki yüz hata kaydı okunur
        std::wstring girdilerMetni = L"["; // JSON dizisi açılır
        for (size_t i = 0; i < liste.size(); i++) // Her kayıt sırayla gezilir
        {
            if (i > 0) { girdilerMetni += L","; } // Kayıtlar arasına virgül eklenir
            girdilerMetni += L"{\"tarih\":\"" + JsonIcinKacisla(liste[i].tarih) + L"\",\"saat\":\"" + JsonIcinKacisla(liste[i].saat) + L"\",\"kod\":\"" + JsonIcinKacisla(liste[i].kod) + L"\",\"mesaj\":\"" + JsonIcinKacisla(liste[i].mesaj) + L"\",\"aciklama\":\"" + JsonIcinKacisla(Gunluk::KoduAciklamayaCevir(liste[i].kod)) + L"\"}"; // Kodun açıklaması da ayrıca eklenir, arayüzün tekrar sormasına gerek kalmaz
        }
        girdilerMetni += L"]"; // JSON dizisi kapatılır
        SekmeyeMesajGonder(sekmeId, L"{\"type\":\"errorlog\",\"entries\":" + girdilerMetni + L"}"); // Hazırlanan mesaj gönderilir
    }
    else if (komut == L"about") // Hakkında bilgisi (RAM kullanımı, çalışma süresi) isteniyorsa
    {
        DWORD ramMB = ToplamRamKullanimiMB(); // Tüm ilgili süreçlerin topladığı RAM, megabayt cinsinden hesaplanır
        ULONGLONG calismaSuresiSaniye = (GetTickCount64() - m_baslangicZamani) / 1000; // Programın açık kaldığı süre saniye cinsinden hesaplanır
        std::wstring mesaj = L"{\"type\":\"about\",\"ramMB\":" + std::to_wstring(ramMB) + L",\"uptimeSeconds\":" + std::to_wstring(calismaSuresiSaniye) + L",\"tabCount\":" + std::to_wstring(m_sekmeler.size()) + L"}"; // Hakkında bilgisi tek bir JSON nesnesinde toplanır
        SekmeyeMesajGonder(sekmeId, mesaj); // Hazırlanan mesaj gönderilir
    }
    else if (komut == L"passwords") // Kayıtlı şifrelerin listesi isteniyorsa
    {
        std::vector<SifreKaydi> liste = SifreKasasi::SifreleriOku(); // Şifreler diskten okunur (hâlâ şifreli haldedir)
        std::wstring girdilerMetni = L"["; // JSON dizisi açılır
        for (size_t i = 0; i < liste.size(); i++) // Her kayıt sırayla gezilir
        {
            if (i > 0) { girdilerMetni += L","; } // Kayıtlar arasına virgül eklenir
            // Dikkat: sifreliVeri alanı kasıtlı olarak bu mesaja hiç dahil edilmez, arayüz şifrenin kendisini sadece reveal_password isteğiyle ve Windows doğrulamasından sonra görebilir
            girdilerMetni += L"{\"site\":\"" + JsonIcinKacisla(liste[i].site) + L"\",\"username\":\"" + JsonIcinKacisla(liste[i].kullaniciAdi) + L"\",\"addedAt\":\"" + JsonIcinKacisla(liste[i].eklenmeZamani) + L"\"}";
        }
        girdilerMetni += L"]"; // JSON dizisi kapatılır
        SekmeyeMesajGonder(sekmeId, L"{\"type\":\"passwords\",\"entries\":" + girdilerMetni + L"}"); // Hazırlanan mesaj gönderilir
    }
    else if (komut == L"permissions") // Kaydedilmiş izin kararlarının listesi isteniyorsa
    {
        std::vector<IzinKaydi> liste = IzinKasasi::TumKararlariOku(); // İzin kararları diskten okunur
        std::wstring girdilerMetni = L"["; // JSON dizisi açılır
        for (size_t i = 0; i < liste.size(); i++) // Her kayıt sırayla gezilir
        {
            if (i > 0) { girdilerMetni += L","; } // Kayıtlar arasına virgül eklenir
            girdilerMetni += L"{\"kaynak\":\"" + JsonIcinKacisla(liste[i].kaynak) + L"\",\"tur\":\"" + JsonIcinKacisla(liste[i].tur) + L"\",\"izinVerildi\":" + (liste[i].izinVerildi ? L"true" : L"false") + L"}";
        }
        girdilerMetni += L"]"; // JSON dizisi kapatılır
        SekmeyeMesajGonder(sekmeId, L"{\"type\":\"permissions\",\"entries\":" + girdilerMetni + L"}"); // Hazırlanan mesaj gönderilir
    }
}

// Bu fonksiyon, verilen bir zaman noktasını (time_t, UNIX başlangıcından bu yana geçen saniye), Storage.cpp'nin kullandığı ISO 8601 metin biçimine (YYYY-MM-DDTHH:MM:SSZ) çevirir
// Bu çevrim, WebView2'nin zaman aralığı API'si ile bizim kendi geçmiş dosyamızın aynı kesim noktasını kullanmasını sağlamak için gereklidir
static std::wstring ZamanTIsoyaCevir(time_t zaman)
{
    std::tm parcalanmisZaman{}; // Yıl/ay/gün/saat gibi parçalara ayrılmış zamanı tutacak yapı
    gmtime_s(&parcalanmisZaman, &zaman); // Verilen zaman, yerel saat dilimine değil, doğrudan UTC'ye göre parçalanır
    wchar_t tamponMetin[32]; // ISO metninin yazılacağı tampon
    swprintf_s(tamponMetin, L"%04d-%02d-%02dT%02d:%02d:%02dZ", parcalanmisZaman.tm_year + 1900, parcalanmisZaman.tm_mon + 1, parcalanmisZaman.tm_mday, parcalanmisZaman.tm_hour, parcalanmisZaman.tm_min, parcalanmisZaman.tm_sec); // Yıl 1900'den başladığı için eklenir, ay sıfırdan başladığı için bir eklenir
    return tamponMetin; // Hazırlanan ISO metni döndürülür
}

// Bu fonksiyon, Gizlilik panelindeki veri temizleme işlevi tarafından çağrılır, verilen zaman aralığı ve kategorilere göre hem WebView2'nin kendi verilerini hem de bizim kendi tuttuğumuz geçmiş ve şifre kayıtlarını temizler
void BrowserWindow::VeriTemizle(int sekmeId, const std::wstring& zamanKodu, const std::wstring& kategorilerMetni)
{
    time_t simdi = time(nullptr); // Şu anki zaman, temizlenecek aralığın bitiş noktası olarak kullanılır
    time_t baslangic = 0; // Temizlenecek aralığın başlangıç noktası, "tum" (tüm zamanlar) için sıfır (UNIX başlangıcı) olarak kalır
    if (zamanKodu == L"1s") { baslangic = simdi - 3600; } // Son bir saat
    else if (zamanKodu == L"24s") { baslangic = simdi - 86400; } // Son yirmi dört saat
    else if (zamanKodu == L"7g") { baslangic = simdi - (7 * 86400); } // Son yedi gün
    else if (zamanKodu == L"4h") { baslangic = simdi - (28 * 86400); } // Son dört hafta (yirmi sekiz gün)
    // "tum" değeri veya tanınmayan bir kod gelirse, başlangıç sıfırda kalır ve tüm zamanlar temizlenmiş olur

    // Seçilen kategori adlarına göre, WebView2'nin kendi verilerinden hangilerinin silineceğini belirten bayraklar birleştirilir
    COREWEBVIEW2_BROWSING_DATA_KINDS bayraklar = static_cast<COREWEBVIEW2_BROWSING_DATA_KINDS>(0);
    bool gecmisSeciliMi = kategorilerMetni.find(L"gecmis") != std::wstring::npos;
    bool sifreSeciliMi = kategorilerMetni.find(L"sifre") != std::wstring::npos;
    if (gecmisSeciliMi) { bayraklar = bayraklar | COREWEBVIEW2_BROWSING_DATA_KINDS_BROWSING_HISTORY; }
    if (kategorilerMetni.find(L"cerez") != std::wstring::npos) { bayraklar = bayraklar | COREWEBVIEW2_BROWSING_DATA_KINDS_COOKIES; }
    if (kategorilerMetni.find(L"onbellek") != std::wstring::npos) { bayraklar = bayraklar | COREWEBVIEW2_BROWSING_DATA_KINDS_CACHE_STORAGE | COREWEBVIEW2_BROWSING_DATA_KINDS_DISK_CACHE; }
    if (kategorilerMetni.find(L"indirme") != std::wstring::npos) { bayraklar = bayraklar | COREWEBVIEW2_BROWSING_DATA_KINDS_DOWNLOAD_HISTORY; } // Bu sadece indirme kaydını siler, diske inmiş dosyaları silmez
    if (kategorilerMetni.find(L"doldurma") != std::wstring::npos) { bayraklar = bayraklar | COREWEBVIEW2_BROWSING_DATA_KINDS_GENERAL_AUTOFILL; OtomatikDoldurmaKasasi::TumVeriyiSil(); } // WebView2'nin kendi otomatik doldurma önbelleğiyle birlikte, bizim kendi kişisel bilgi profilimiz ve kayıtlı kartlarımız da silinir
    if (kategorilerMetni.find(L"izinler") != std::wstring::npos) { bayraklar = bayraklar | COREWEBVIEW2_BROWSING_DATA_KINDS_SETTINGS; }
    // "sifre" kategorisi WebView2'nin kendi bayraklarında yoktur, çünkü bu program kendi DPAPI şifreli kasasını kullanır, WebView2'nin yerleşik şifre doldurma sistemini kullanmaz, bu yüzden ayrıca SifreKasasi üzerinden temizlenir

    if (bayraklar != 0 && m_toolbarWebView != nullptr) // Temizlenecek en az bir WebView2 kategorisi seçildiyse ve toolbar WebView2'si hazırsa
    {
        ComPtr<ICoreWebView2_13> webViewSurum13; // get_Profile fonksiyonu bu arayüz sürümünde bulunduğu için
        m_toolbarWebView.As(&webViewSurum13);
        if (webViewSurum13 != nullptr)
        {
            ComPtr<ICoreWebView2Profile> profil; // Toolbar'ın kullandığı profil, tüm sekmelerle aynı paylaşılan ortamdan geldiği için her sekmenin verisini de kapsar
            webViewSurum13->get_Profile(&profil);
            ComPtr<ICoreWebView2Profile2> profilSurum2; // ClearBrowsingDataInTimeRange fonksiyonu bu arayüz sürümünde bulunduğu için
            if (profil != nullptr) { profil.As(&profilSurum2); }
            if (profilSurum2 != nullptr)
            {
                profilSurum2->ClearBrowsingDataInTimeRange(
                    bayraklar,
                    static_cast<double>(baslangic), // WebView2, bu zamanları UNIX başlangıcından bu yana geçen saniye olarak bekler
                    static_cast<double>(simdi),
                    Callback<ICoreWebView2ClearBrowsingDataCompletedHandler>(
                        [](HRESULT sonuc) -> HRESULT { return S_OK; } // Temizleme arka planda tamamlanır, arayüze ayrıca bir bildirim göndermeye gerek yoktur
                    ).Get()
                );
            }
        }
    }

    if (gecmisSeciliMi) // Eğer "Gezinme geçmişi" kategorisi seçildiyse, bizim kendi history.json dosyamız da aynı zaman aralığına göre temizlenir
    {
        Depolama::GecmisiTemizle(zamanKodu == L"tum" ? L"" : ZamanTIsoyaCevir(baslangic));
    }
    if (sifreSeciliMi) // Eğer "Şifreler" kategorisi seçildiyse, DPAPI şifre kasası tamamen temizlenir (zaman aralığı şifre kayıtlarına uygulanmaz, hepsi birden silinir)
    {
        SifreKasasi::TumSifreleriSil();
    }

    // Temizlenen veriler, arayüzde hâlâ eski haliyle görünmesin diye, ilgili listeler güncel haliyle tekrar gönderilir
    AyarlarIcinVeriGonder(sekmeId, L"history");
    AyarlarIcinVeriGonder(sekmeId, L"passwords");
}

// Bu fonksiyon, fare bir sekmenin üzerinde beklerken çağrılır, verilen sekmenin güncel görünümünün küçük bir anlık görüntüsünü (PNG) yakalayıp, Base64 ile kodlanmış halde toolbar arayüzüne gönderir
void BrowserWindow::SekmeOnizlemesiGonder(int sekmeId)
{
    SekmeBilgisi* sekme = SekmeBul(sekmeId); // İstenen önizlemeye ait sekme bulunur
    if (sekme == nullptr || sekme->webView == nullptr || sekme->askidaMi) // Eğer sekme yoksa, denetleyicisi henüz hazır değilse veya sekme uykudaysa (ekranda çizili bir yüzeyi olmadığı için)
    {
        return; // Önizleme alınamaz, sessizce vazgeçilir, arayüz tarafı zaten cevap gelmeyince bir şey göstermeyecektir
    }

    ComPtr<IStream> akis; // Yakalanan PNG baytlarının yazılacağı, bellekte büyüyebilen bir akış
    CreateStreamOnHGlobal(nullptr, TRUE, &akis); // Akış, dosyaya değil doğrudan belleğe yazacak şekilde oluşturulur, TRUE parametresi akış kapatılınca belleğin de serbest bırakılmasını sağlar

    sekme->webView->CapturePreview(
        COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_PNG,
        akis.Get(),
        Callback<ICoreWebView2CapturePreviewCompletedHandler>(
            [this, sekmeId, akis, gecerlilikBayragi = m_pencereGecerliMi](HRESULT sonuc) -> HRESULT
            {
                if (!*gecerlilikBayragi) // Pencere, bu asenkron yakalama tamamlanmadan önce kapatılmaya başladıysa
                {
                    return S_OK; // "this" işaretçisine hiç dokunulmadan güvenle vazgeçilir
                }
                if (FAILED(sonuc)) // Yakalama başarısız olduysa (örneğin sekme bu sırada kapatıldıysa)
                {
                    return S_OK; // Sessizce vazgeçilir
                }

                LARGE_INTEGER sifirKonum{}; // Akış, yazma sırasında sona ilerlediği için okumadan önce başa sarılması gerekir
                akis->Seek(sifirKonum, STREAM_SEEK_SET, nullptr);

                STATSTG istatistik{}; // Akışın boyutunu öğrenmek için kullanılan bilgi yapısı
                akis->Stat(&istatistik, STATFLAG_NONAME);
                ULONG boyut = static_cast<ULONG>(istatistik.cbSize.QuadPart); // Önizleme görüntüleri küçük olduğu için boyutun 32 bit sınırını aşması beklenmez

                std::vector<uint8_t> hamVeri(boyut); // Okunacak PNG baytları için bir arabellek ayrılır
                ULONG okunanBayt = 0; // Gerçekte kaç bayt okunduğunu tutacak değişken
                akis->Read(hamVeri.data(), boyut, &okunanBayt);
                hamVeri.resize(okunanBayt); // Beklenenden az okunmuşsa (olağan değildir ama yine de), arabellek gerçek boyuta indirilir

                std::wstring base64Metin = Base64Kodla(hamVeri); // Ham PNG baytları, JSON içinde taşınabilecek bir metne çevrilir
                std::wstring mesaj = L"{\"type\":\"tabpreview\",\"id\":" + std::to_wstring(sekmeId) + L",\"image\":\"data:image/png;base64," + base64Metin + L"\"}"; // Hazır veri adresi (data URL) biçiminde, doğrudan bir <img> öğesine atanabilecek şekilde hazırlanır
                ToolbaraMesajGonder(mesaj); // Hazırlanan mesaj, önizlemeyi isteyen toolbar arayüzüne gönderilir

                return S_OK; // İşlem başarıyla ele alındı
            }
        ).Get()
    );
}

// Bu fonksiyon, dahili bir Ayarlar sekmesi açar, eğer zaten açıksa var olan sekmeye geçer
void BrowserWindow::AyarlarSekmesiniAc(const std::wstring& kategori)
{
    std::wstring hedefAdres = L"https://tarayici.local/settings.html" + (kategori.empty() ? L"" : L"#" + kategori); // İstenen kategori varsa, adresin sonuna bir URL parçası (hash) olarak eklenir, settings.js sayfa açılırken bunu okuyup doğru paneli gösterir

    if (m_ayarlarSekmeId != -1 && SekmeBul(m_ayarlarSekmeId) != nullptr) // Eğer zaten açık bir Ayarlar sekmesi varsa
    {
        if (!kategori.empty()) // Belirli bir kategoriye gidilmesi isteniyorsa
        {
            SekmeBilgisi* mevcutSekme = SekmeBul(m_ayarlarSekmeId); // Var olan Ayarlar sekmesinin bilgilerine erişilir
            if (mevcutSekme != nullptr && mevcutSekme->webView != nullptr) // Denetleyicisi hazırsa
            {
                mevcutSekme->webView->Navigate(hedefAdres.c_str()); // Aynı sayfa içinde sadece hash değiştiği için bu gezinme neredeyse anında tamamlanır, sayfa yeniden yüklenmez
            }
        }
        SekmeyeGec(m_ayarlarSekmeId); // Var olan Ayarlar sekmesine geçiş yapılır, yenisi açılmaz
        return; // Fonksiyondan çıkılır
    }

    m_ayarlarSekmeId = YeniSekmeOlustur(hedefAdres, true, true); // Yeni, dahili bir Ayarlar sekmesi açılır, kimlik numarası hatırlanır
}

// Bu fonksiyon, güncel tema ayarlarını toolbar arayüzüne gönderir
void BrowserWindow::TemayiToolbaraGonder()
{
    AyarVerisi a = Ayarlar::Oku(); // Güncel ayarlar diskten okunur
    std::wstring mesaj = L"{\"type\":\"theme\",\"theme\":" + TemaNesnesiJsonYap(a) + L"}"; // Tema bilgisi bir JSON nesnesi olarak hazırlanır
    ToolbaraMesajGonder(mesaj); // Hazırlanan mesaj toolbar arayüzüne gönderilir
    MenuyeMesajGonder(mesaj); // Hamburger menü paneli, kendi ayrı sayfası olduğu için toolbar'dan habersizdir, bu yüzden aynı mesaj ona da ayrıca gönderilir
}

// Bu fonksiyon, kayıtlı yer imlerini okuyup toolbar arayüzündeki yer imi çubuğuna gönderir
void BrowserWindow::BookmarkBariGuncelleVeGonder()
{
    std::vector<SayfaKaydi> liste = Depolama::YerImleriniOku(); // Yer imleri diskten okunur
    std::wstring mesaj = L"{\"type\":\"bookmarkbar\",\"entries\":" + SayfaKayitlariniJsonYap(liste) + L"}"; // Liste, bir JSON dizisi olarak sarmalanır
    ToolbaraMesajGonder(mesaj); // Hazırlanan mesaj toolbar arayüzüne gönderilir
}

// Bu fonksiyon, basılan bir klavye kısayolunun tarayıcıya özel bir komuta karşılık gelip gelmediğini kontrol eder, karşılık geliyorsa ilgili işlemi yapar
bool BrowserWindow::KisayolTusuIsle(UINT32 tusKodu)
{
    if (tusKodu == VK_F12) // F12, diğer tüm kısayolların aksine Ctrl gerektirmez, bu yüzden Ctrl kontrolünden önce ayrıca ele alınır
    {
        DevToolsAcKapat(); // Gömülü DevTools paneli açılır/kapatılır
        return true; // WebView2'nin kendi varsayılan F12 davranışı (ayrı bir pencere açması) böylece engellenir
    }

    bool ctrlBasili = (GetKeyState(VK_CONTROL) & 0x8000) != 0; // Şu anda Ctrl tuşunun basılı olup olmadığı kontrol edilir
    bool shiftBasili = (GetKeyState(VK_SHIFT) & 0x8000) != 0; // Şu anda Shift tuşunun basılı olup olmadığı kontrol edilir

    if (!ctrlBasili) // Tüm kısayollarımız Ctrl tuşuyla birlikte basıldığı için, Ctrl basılı değilse
    {
        return false; // Bu tuş bizim bir kısayolumuz değildir
    }

    SekmeBilgisi* aktifSekme = AktifSekmeyiAl(); // Çoğu kısayol aktif sekme üzerinde çalıştığı için, aktif sekme bilgisi baştan alınır

    // Aşağıdaki yedi kısayolun hangi harfe atandığı, Ayarlar sayfasından özelleştirilebilir, bu yüzden sabit bir harf yerine kullanıcının güncel tercihi okunur
    AyarVerisi guncelAyarlar = Ayarlar::Oku();

    if (tusKodu == static_cast<UINT32>(guncelAyarlar.kisayolYeniSekme) && !shiftBasili) // Ctrl+(özelleştirilebilir), yeni sekme açar
    {
        YeniSekmeOlustur(L"https://tarayici.local/newtab.html", true, true);
        return true;
    }
    if (tusKodu == static_cast<UINT32>(guncelAyarlar.kisayolYeniSekme) && shiftBasili) // Ctrl+Shift+(özelleştirilebilir), en son kapatılan sekmeyi yeniden açar
    {
        SonKapatilanSekmeyiAc();
        return true;
    }
    if (tusKodu == static_cast<UINT32>(guncelAyarlar.kisayolCogalt) && !shiftBasili) // Ctrl+(özelleştirilebilir), aktif sekmeyi çoğaltır
    {
        if (aktifSekme != nullptr) { SekmeyiCogalt(aktifSekme->id); }
        return true;
    }
    if (tusKodu == static_cast<UINT32>(guncelAyarlar.kisayolYazdir) && !shiftBasili) // Ctrl+(özelleştirilebilir), aktif sayfayı yazdırma/PDF kaydetme penceresini açar
    {
        SayfayiYazdir();
        return true;
    }
    if (tusKodu == 'N' && shiftBasili) // Ctrl+Shift+N, yeni gizli sekme açar
    {
        GizliSekmeAc();
        return true;
    }
    if (tusKodu == 'U' && !shiftBasili) // Ctrl+U, aktif sayfanın ham HTML kaynağını yeni bir sekmede gösterir, bu kısayol sabittir, özelleştirilebilir değildir
    {
        if (aktifSekme != nullptr && aktifSekme->webView != nullptr && !aktifSekme->dahiliSayfaMi) // Dahili sayfaların (Ayarlar gibi) kaynağını göstermenin bir anlamı yoktur
        {
            LPWSTR adresPtr = nullptr;
            aktifSekme->webView->get_Source(&adresPtr);
            std::wstring adres = adresPtr ? adresPtr : L"";
            if (adresPtr != nullptr) { CoTaskMemFree(adresPtr); }
            if (!adres.empty()) { YeniSekmeOlustur(L"view-source:" + adres, true, false); } // Chromium'un yerleşik "view-source:" şeması kullanılır, WebView2 bunu normal bir sekme içinde destekler
        }
        return true;
    }
    if (tusKodu == static_cast<UINT32>(guncelAyarlar.kisayolSekmeKapat) && !shiftBasili) // Ctrl+(özelleştirilebilir), aktif sekmeyi kapatır
    {
        if (aktifSekme != nullptr) { SekmeKapat(aktifSekme->id); }
        return true;
    }
    if (tusKodu == static_cast<UINT32>(guncelAyarlar.kisayolAdres)) // Ctrl+(özelleştirilebilir), adres çubuğuna odaklanır
    {
        ToolbaraMesajGonder(L"{\"type\":\"focusaddress\"}");
        return true;
    }
    if (tusKodu == static_cast<UINT32>(guncelAyarlar.kisayolYenile) && !shiftBasili) // Ctrl+(özelleştirilebilir), aktif sekmeyi yeniden yükler
    {
        if (aktifSekme != nullptr) { aktifSekme->webView->Reload(); }
        return true;
    }
    if (tusKodu == static_cast<UINT32>(guncelAyarlar.kisayolSayfadaBul) && !shiftBasili) // Ctrl+(özelleştirilebilir), sayfa içi arama çubuğunu açar/kapatır
    {
        SayfaIciAramaCubuguAcKapat();
        return true;
    }
    if (tusKodu == 'A' && shiftBasili) // Ctrl+Shift+A, sekme arama kutusuna odaklanır
    {
        ToolbaraMesajGonder(L"{\"type\":\"focustabsearch\"}");
        return true;
    }
    if (tusKodu == VK_OEM_2 && !shiftBasili) // Ctrl+/, klavye kısayolları sekmesini açar
    {
        KisayollarSekmesiniAc();
        return true;
    }
    if (tusKodu == VK_OEM_PLUS) // Ctrl+= (veya Ctrl+Shift+ ile Ctrl+artı), aktif sekmeyi yakınlaştırır
    {
        ZoomDegistir(aktifSekme, 0.1);
        return true;
    }
    if (tusKodu == VK_OEM_MINUS) // Ctrl+-, aktif sekmeyi uzaklaştırır
    {
        ZoomDegistir(aktifSekme, -0.1);
        return true;
    }
    if (tusKodu == '0') // Ctrl+0, aktif sekmenin yakınlaştırmasını varsayılana sıfırlar
    {
        if (aktifSekme != nullptr) { aktifSekme->denetleyici->put_ZoomFactor(1.0); }
        return true;
    }

    return false; // Bu tuş, tanınan kısayollardan hiçbirine uymuyor
}

// Bu fonksiyon, verilen sekmenin yakınlaştırma oranını, verilen miktar kadar artırır veya azaltır
void BrowserWindow::ZoomDegistir(SekmeBilgisi* sekme, double miktar)
{
    if (sekme == nullptr) // Eğer değiştirilecek bir sekme yoksa
    {
        return; // Yapılacak bir şey yoktur
    }

    double guncelOran = 1.0; // Sekmenin şu anki yakınlaştırma oranını tutacak değişken
    sekme->denetleyici->get_ZoomFactor(&guncelOran); // Güncel oran okunur

    double yeniOran = guncelOran + miktar; // Yeni oran hesaplanır
    if (yeniOran < 0.25) { yeniOran = 0.25; } // Oranın aşırı küçülmesi engellenir
    if (yeniOran > 4.0) { yeniOran = 4.0; } // Oranın aşırı büyümesi engellenir

    sekme->denetleyici->put_ZoomFactor(yeniOran); // Yeni oran uygulanır
}

// Bu fonksiyon, aktif sekmenin içine, basit bir sayfa içi arama çubuğu enjekte eder, çubuk zaten açıksa kapatır
void BrowserWindow::SayfaIciAramaCubuguAcKapat()
{
    SekmeBilgisi* aktifSekme = AktifSekmeyiAl(); // Arama çubuğunun ekleneceği sekme, aktif sekme olarak alınır
    if (aktifSekme == nullptr) // Hiç aktif sekme yoksa
    {
        return; // Arama çubuğu eklemeye gerek yoktur
    }

    // Bu JavaScript kodu, sayfanın sağ üst köşesine küçük bir arama kutusu ekler, tarayıcının kendi window.find fonksiyonunu kullanarak eşleşmeleri vurgular ve ekrana kaydırır, eğer kutu zaten açıksa onu kaldırır
    const wchar_t* betik = LR"JS(
(function() {
    var kutu = document.getElementById('tarayici-arama-cubugu');
    if (kutu) { kutu.remove(); try { window.getSelection().removeAllRanges(); } catch (e) {} return; }
    kutu = document.createElement('div');
    kutu.id = 'tarayici-arama-cubugu';
    kutu.style.cssText = 'position:fixed;top:10px;right:10px;z-index:2147483647;background:#1a1b22;color:#e8e8ea;padding:8px;border-radius:8px;font-family:Segoe UI,Arial,sans-serif;box-shadow:0 2px 8px rgba(0,0,0,0.5);';
    kutu.innerHTML = '<input id="tarayici-arama-girdi" style="padding:4px 8px;border-radius:4px;border:1px solid #444;background:#2c2d36;color:#fff;outline:none;" placeholder="Sayfada ara" /> <button id="tarayici-arama-sonraki" style="cursor:pointer;">Sonraki</button> <button id="tarayici-arama-kapat" style="cursor:pointer;">Kapat</button>';
    document.body.appendChild(kutu);
    var girdi = document.getElementById('tarayici-arama-girdi');
    function ara() { try { window.find(girdi.value, false, false, true, false, true, false); } catch (e) {} }
    girdi.addEventListener('keydown', function (e) { if (e.key === 'Enter') { ara(); } if (e.key === 'Escape') { kutu.remove(); } });
    document.getElementById('tarayici-arama-sonraki').addEventListener('click', ara);
    document.getElementById('tarayici-arama-kapat').addEventListener('click', function () { kutu.remove(); });
    girdi.focus();
})();
)JS";

    aktifSekme->webView->ExecuteScript(betik, nullptr); // Hazırlanan betik, aktif sekmenin içinde çalıştırılır
}

// Bu fonksiyon, aktif sekmenin görünür metnini sesli okumaya başlar/durdurur
void BrowserWindow::SesliOkumayiBaslatDurdur()
{
    SekmeBilgisi* aktifSekme = AktifSekmeyiAl();
    if (aktifSekme == nullptr)
    {
        return;
    }

    // Bu betik, sayfada zaten bir okuma kontrol çubuğu varsa, okumayı durdurur ve çubuğu kaldırır
    // Yoksa, sayfanın görünür metnini (document.body.innerText) alıp, tarayıcının kendi Web Speech API'siyle (window.speechSynthesis) Türkçe olarak okumaya başlar, küçük bir Duraklat/Devam/Durdur çubuğu ekler
    const wchar_t* betik = LR"JS(
(function() {
    var cubuk = document.getElementById('tarayici-sesli-okuma-cubugu');
    if (cubuk) { window.speechSynthesis.cancel(); cubuk.remove(); return; }

    var metin = (document.body.innerText || '').trim();
    if (!metin) { return; }

    var konusma = new SpeechSynthesisUtterance(metin);
    konusma.lang = 'tr-TR';
    window.speechSynthesis.cancel();
    window.speechSynthesis.speak(konusma);

    cubuk = document.createElement('div');
    cubuk.id = 'tarayici-sesli-okuma-cubugu';
    cubuk.style.cssText = 'position:fixed;bottom:16px;right:16px;z-index:2147483647;background:#1a1b22;color:#e8e8ea;padding:10px 14px;border-radius:10px;font-family:Segoe UI,Arial,sans-serif;box-shadow:0 2px 10px rgba(0,0,0,0.5);display:flex;gap:8px;align-items:center;';
    cubuk.innerHTML = '🔊 Sesli Okuma <button id="tarayici-ses-duraklat" style="cursor:pointer;">Duraklat</button> <button id="tarayici-ses-durdur" style="cursor:pointer;">Durdur</button>';
    document.body.appendChild(cubuk);

    document.getElementById('tarayici-ses-duraklat').addEventListener('click', function (e) {
        if (window.speechSynthesis.paused) { window.speechSynthesis.resume(); e.target.textContent = 'Duraklat'; }
        else { window.speechSynthesis.pause(); e.target.textContent = 'Devam Et'; }
    });
    document.getElementById('tarayici-ses-durdur').addEventListener('click', function () {
        window.speechSynthesis.cancel();
        cubuk.remove();
    });

    konusma.onend = function () { var c = document.getElementById('tarayici-sesli-okuma-cubugu'); if (c) { c.remove(); } };
})();
)JS";

    aktifSekme->webView->ExecuteScript(betik, nullptr);
}

// Bu fonksiyon, aktif sekmedeki sayfanın ana metin içeriğini bulup, sade bir okuma görünümünde gösterir
void BrowserWindow::OkuyucuModunuAc()
{
    SekmeBilgisi* aktifSekme = AktifSekmeyiAl();
    if (aktifSekme == nullptr)
    {
        return;
    }

    // Bu betik, sayfadaki olası ana içerik öğelerini (article, main, veya en çok paragraf metni içeren div) sezgisel olarak karşılaştırır, en uygun olanı seçer
    // Seçilen öğenin bir kopyası (klon) alınır, bu kopyadan script/style/nav/reklam gibi dikkat dağıtıcı öğeler temizlenir, ve sonuç tam sayfayı kaplayan sade, büyük puntolu bir okuma görünümünde gösterilir
    // Okuyucu görünümü zaten açıksa, tekrar çağrıldığında kapatılır (aç/kapa anahtarı gibi çalışır)
    const wchar_t* betik = LR"JS(
(function() {
    var varOlan = document.getElementById('tarayici-okuyucu-modu');
    if (varOlan) { varOlan.remove(); document.documentElement.style.overflow = ''; return; }

    var adaylar = Array.from(document.querySelectorAll('article, main, [role=main], div, section'));
    var enIyi = null, enIyiPuan = 0;
    adaylar.forEach(function (aday) {
        var paragraflar = aday.querySelectorAll('p');
        var puan = 0;
        paragraflar.forEach(function (p) { puan += (p.innerText || '').length; });
        if (paragraflar.length < 2) { puan = 0; } // En az iki paragrafı olmayan öğeler, "ana içerik" olarak sayılmaz
        if (puan > enIyiPuan) { enIyiPuan = puan; enIyi = aday; }
    });
    if (!enIyi || enIyiPuan < 200) { enIyi = document.body; } // Yeterince güçlü bir aday bulunamazsa, tüm sayfa gövdesi kullanılır

    var klon = enIyi.cloneNode(true);
    klon.querySelectorAll('script, style, iframe, noscript, button, input, nav, footer, aside, [class*=ad], [id*=ad], form, svg').forEach(function (e) { e.remove(); });

    var baslikMetni = (document.querySelector('h1') ? document.querySelector('h1').innerText : document.title) || '';

    var kapsayici = document.createElement('div');
    kapsayici.id = 'tarayici-okuyucu-modu';
    kapsayici.style.cssText = 'position:fixed;inset:0;z-index:2147483647;background:#1a1b22;color:#e8e8ea;overflow-y:auto;font-family:Segoe UI,Arial,sans-serif;';
    kapsayici.innerHTML =
        '<div style="max-width:700px;margin:0 auto;padding:48px 24px 80px;line-height:1.7;font-size:18px;">' +
        '<button id="tarayici-okuyucu-kapat" style="position:fixed;top:16px;right:16px;cursor:pointer;background:#2c2d36;color:#fff;border:none;border-radius:8px;padding:8px 14px;">✕ Kapat</button>' +
        '<h1 style="font-size:28px;margin-bottom:24px;">' + baslikMetni.replace(/</g, '&lt;') + '</h1>' +
        '<div id="tarayici-okuyucu-govde"></div>' +
        '</div>';
    kapsayici.querySelector('#tarayici-okuyucu-govde').appendChild(klon);
    document.documentElement.style.overflow = 'hidden';
    document.body.appendChild(kapsayici);

    document.getElementById('tarayici-okuyucu-kapat').addEventListener('click', function () {
        kapsayici.remove();
        document.documentElement.style.overflow = '';
    });
})();
)JS";

    aktifSekme->webView->ExecuteScript(betik, nullptr);
}

// Bu fonksiyon, aktif sekmedeki sayfayı zorla koyu renklere çevirir, tekrar çağrılırsa geri alır
void BrowserWindow::ZorunluKoyuModuAcKapat()
{
    SekmeBilgisi* aktifSekme = AktifSekmeyiAl();
    if (aktifSekme == nullptr)
    {
        return;
    }

    // Bu klasik yöntem, tüm sayfanın renklerini tersine çevirip (invert) ardından renk tonunu 180 derece döndürür (hue-rotate), bu ikisinin birleşimi açık bir sayfayı koyu bir sayfaya çevirirken renklerin "yanlış" görünmesini (örneğin mavinin turuncu olması) önler
    // Görseller, videolar ve zaten arka plan resmi olan öğeler ise AYRICA bir kez daha tersine çevrilir, böylece onlar iki kez ters çevrilip normal (doğru) hallerine geri döner, sadece metin/arka plan gibi düz renkler koyulaşmış olarak kalır
    const wchar_t* betik = LR"JS(
(function() {
    var mevcutStil = document.getElementById('tarayici-zorunlu-koyu-mod');
    if (mevcutStil) { mevcutStil.remove(); return; }

    var stil = document.createElement('style');
    stil.id = 'tarayici-zorunlu-koyu-mod';
    stil.textContent =
        'html { filter: invert(1) hue-rotate(180deg) !important; background: #fff !important; }' +
        'img, video, picture, canvas, svg, iframe { filter: invert(1) hue-rotate(180deg) !important; }';
    document.documentElement.appendChild(stil);
})();
)JS";

    aktifSekme->webView->ExecuteScript(betik, nullptr);
}

// Bu fonksiyon, profili (uzantı yönetimi fonksiyonlarının bulunduğu arayüz sürümü) toolbar'ın WebView2'si üzerinden bulur, tüm sekmeler aynı ortamı/profili paylaştığı için hangi WebView2 üzerinden alındığı önemli değildir
static ComPtr<ICoreWebView2Profile7> EklentiProfiliniAl(ICoreWebView2* herhangiBirWebView)
{
    ComPtr<ICoreWebView2Profile7> profilSurum7;
    if (herhangiBirWebView == nullptr) { return profilSurum7; }

    ComPtr<ICoreWebView2_13> webViewSurum13;
    herhangiBirWebView->QueryInterface(IID_PPV_ARGS(&webViewSurum13));
    if (webViewSurum13 == nullptr) { return profilSurum7; }

    ComPtr<ICoreWebView2Profile> profil;
    webViewSurum13->get_Profile(&profil);
    if (profil == nullptr) { return profilSurum7; }

    profil.As(&profilSurum7);
    return profilSurum7;
}

// Bu fonksiyon, yüklü tüm paketlenmemiş tarayıcı uzantılarının listesini verilen sekmeye JSON olarak gönderir
void BrowserWindow::EklentileriniGonder(int sekmeId)
{
    ComPtr<ICoreWebView2Profile7> profil = EklentiProfiliniAl(m_toolbarWebView.Get());
    if (profil == nullptr)
    {
        SekmeyeMesajGonder(sekmeId, L"{\"type\":\"extensions\",\"entries\":[]}");
        return;
    }

    profil->GetBrowserExtensions(
        Callback<ICoreWebView2ProfileGetBrowserExtensionsCompletedHandler>(
            [this, sekmeId, gecerlilikBayragi = m_pencereGecerliMi](HRESULT sonuc, ICoreWebView2BrowserExtensionList* liste) -> HRESULT
            {
                if (!*gecerlilikBayragi) { return S_OK; }
                std::wstring girdilerMetni = L"[";
                UINT32 sayi = 0;
                if (liste != nullptr) { liste->get_Count(&sayi); }
                for (UINT32 i = 0; i < sayi; i++)
                {
                    ComPtr<ICoreWebView2BrowserExtension> eklenti;
                    liste->GetValueAtIndex(i, &eklenti);
                    if (eklenti == nullptr) { continue; }

                    LPWSTR idPtr = nullptr; eklenti->get_Id(&idPtr);
                    LPWSTR adPtr = nullptr; eklenti->get_Name(&adPtr);
                    BOOL etkinMi = FALSE; eklenti->get_IsEnabled(&etkinMi);

                    if (i > 0) { girdilerMetni += L","; }
                    girdilerMetni += L"{\"id\":\"" + JsonIcinKacisla(idPtr ? idPtr : L"") + L"\",\"name\":\"" + JsonIcinKacisla(adPtr ? adPtr : L"") + L"\",\"enabled\":" + (etkinMi ? L"true" : L"false") + L"}";

                    if (idPtr != nullptr) { CoTaskMemFree(idPtr); }
                    if (adPtr != nullptr) { CoTaskMemFree(adPtr); }
                }
                girdilerMetni += L"]";
                SekmeyeMesajGonder(sekmeId, L"{\"type\":\"extensions\",\"entries\":" + girdilerMetni + L"}");
                return S_OK;
            }
        ).Get()
    );
}

// Bu fonksiyon, kullanıcının bir klasör seçmesini ister, seçilen klasörü paketlenmemiş bir uzantı olarak yüklemeye çalışır
void BrowserWindow::EklentiKlasoruSecVeYukle(int sekmeId)
{
    wchar_t secilenYol[MAX_PATH] = {};

    BROWSEINFOW gozatBilgisi = {}; // Windows'un standart "Klasör Seç" penceresini açmak için gereken ayarlar
    gozatBilgisi.hwndOwner = m_pencere;
    gozatBilgisi.lpszTitle = L"Yüklenecek uzantının klasörünü seçin (manifest.json içeren klasör)";
    gozatBilgisi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST sonuc = SHBrowseForFolderW(&gozatBilgisi);
    if (sonuc == nullptr) // Kullanıcı pencereyi iptal ettiyse
    {
        return;
    }
    SHGetPathFromIDListW(sonuc, secilenYol);
    CoTaskMemFree(sonuc);

    ComPtr<ICoreWebView2Profile7> profil = EklentiProfiliniAl(m_toolbarWebView.Get());
    if (profil == nullptr)
    {
        SekmeyeMesajGonder(sekmeId, L"{\"type\":\"extension_load_result\",\"success\":false}");
        return;
    }

    std::wstring secilenYolKopya = secilenYol;
    profil->AddBrowserExtension(
        secilenYolKopya.c_str(),
        Callback<ICoreWebView2ProfileAddBrowserExtensionCompletedHandler>(
            [this, sekmeId, gecerlilikBayragi = m_pencereGecerliMi](HRESULT sonuc, ICoreWebView2BrowserExtension* eklenti) -> HRESULT
            {
                if (!*gecerlilikBayragi) { return S_OK; }
                bool basarili = SUCCEEDED(sonuc) && eklenti != nullptr;
                SekmeyeMesajGonder(sekmeId, L"{\"type\":\"extension_load_result\",\"success\":" + std::wstring(basarili ? L"true" : L"false") + L"}");
                if (basarili) { EklentileriniGonder(sekmeId); } // Başarılıysa, güncel liste de hemen gönderilir
                return S_OK;
            }
        ).Get()
    );
}

// Bu fonksiyon, verilen tam baytlık bir yanıtı, verilen yerel (127.0.0.1 OLMAYAN, herhangi bir) HTTPS sunucusundan ham bayt dizisi olarak indirir
// YerelHttpGet'ten farkı: metin değil, CRX gibi ikili (binary) bir dosya indirdiği için sonucu bir bayt vektörü olarak döndürür, ve WINHTTP_FLAG_SECURE ile HTTPS kullanır
static std::vector<char> UzakHttpsGetIkili(const std::wstring& host, const std::wstring& yol, int zamanAsimiMs)
{
    std::vector<char> sonuc;
    HINTERNET oturum = WinHttpOpen(L"Tarayici Eklenti Indirici", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (oturum == nullptr) { return sonuc; }
    WinHttpSetTimeouts(oturum, zamanAsimiMs, zamanAsimiMs, zamanAsimiMs, zamanAsimiMs);

    HINTERNET baglanti = WinHttpConnect(oturum, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (baglanti != nullptr)
    {
        HINTERNET istek = WinHttpOpenRequest(baglanti, L"GET", yol.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE); // WinHttp, varsayılan olarak 302 yönlendirmelerini otomatik takip eder, CRX indirme uç noktası tam olarak böyle çalışır
        if (istek != nullptr)
        {
            if (WinHttpSendRequest(istek, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(istek, nullptr))
            {
                DWORD mevcutBoyut = 0;
                do
                {
                    mevcutBoyut = 0;
                    if (!WinHttpQueryDataAvailable(istek, &mevcutBoyut) || mevcutBoyut == 0) { break; }
                    size_t eskiBoyut = sonuc.size();
                    sonuc.resize(eskiBoyut + mevcutBoyut);
                    DWORD okunanBayt = 0;
                    if (WinHttpReadData(istek, sonuc.data() + eskiBoyut, mevcutBoyut, &okunanBayt)) { sonuc.resize(eskiBoyut + okunanBayt); }
                    else { sonuc.resize(eskiBoyut); }
                } while (mevcutBoyut > 0);
            }
            WinHttpCloseHandle(istek);
        }
        WinHttpCloseHandle(baglanti);
    }
    WinHttpCloseHandle(oturum);
    return sonuc;
}

// Bu fonksiyon, bir CRX dosyasının ham baytlarından, içindeki asıl ZIP paketinin başladığı konumu bulur
// CRX2: "Cr24" + sürüm(4) + ortak_anahtar_uzunlugu(4) + imza_uzunlugu(4) + ortak_anahtar + imza + ZIP
// CRX3: "Cr24" + sürüm(4) + başlık_uzunlugu(4) + başlık(protobuf) + ZIP
// Ayrıştırılamazsa (tanınmayan biçim/sürüm) SIZE_MAX döner
static size_t CrxIcindenZipBaslangiciniBul(const std::vector<char>& crxBaytlari)
{
    if (crxBaytlari.size() < 12) { return SIZE_MAX; }
    if (crxBaytlari[0] != 'C' || crxBaytlari[1] != 'r' || crxBaytlari[2] != '2' || crxBaytlari[3] != '4') { return SIZE_MAX; } // "Cr24" sihirli imzası

    auto dortBaytOku = [&crxBaytlari](size_t konum) -> uint32_t
    {
        return static_cast<uint8_t>(crxBaytlari[konum]) | (static_cast<uint8_t>(crxBaytlari[konum + 1]) << 8) | (static_cast<uint8_t>(crxBaytlari[konum + 2]) << 16) | (static_cast<uint8_t>(crxBaytlari[konum + 3]) << 24);
    };

    uint32_t surum = dortBaytOku(4);
    if (surum == 3)
    {
        uint32_t baslikUzunlugu = dortBaytOku(8);
        size_t zipBaslangici = 12 + static_cast<size_t>(baslikUzunlugu);
        return (zipBaslangici <= crxBaytlari.size()) ? zipBaslangici : SIZE_MAX;
    }
    if (surum == 2)
    {
        if (crxBaytlari.size() < 16) { return SIZE_MAX; }
        uint32_t ortakAnahtarUzunlugu = dortBaytOku(8);
        uint32_t imzaUzunlugu = dortBaytOku(12);
        size_t zipBaslangici = 16 + static_cast<size_t>(ortakAnahtarUzunlugu) + static_cast<size_t>(imzaUzunlugu);
        return (zipBaslangici <= crxBaytlari.size()) ? zipBaslangici : SIZE_MAX;
    }
    return SIZE_MAX; // Tanınmayan bir CRX sürümü
}

// Bu fonksiyon, verilen bir Chrome Web Mağazası sayfası adresinden uzantı kimliğini (otuz iki karakterlik, a-p arası harflerden oluşan kimlik) çıkarır, bulamazsa boş metin döndürür
static std::wstring MagazaAdresindenKimlikCikar(const std::wstring& magazaUrl)
{
    // Adres, sondaki "/" veya "?" karakterlerinden arındırılır, sonra son "/" işaretinden sonraki parça kimlik adayı olarak alınır
    std::wstring adres = magazaUrl;
    size_t soruKonumu = adres.find(L'?');
    if (soruKonumu != std::wstring::npos) { adres = adres.substr(0, soruKonumu); }
    while (!adres.empty() && adres.back() == L'/') { adres.pop_back(); }

    size_t sonTersBolu = adres.find_last_of(L'/');
    std::wstring aday = (sonTersBolu == std::wstring::npos) ? adres : adres.substr(sonTersBolu + 1);

    if (aday.size() != 32) { return L""; } // Geçerli bir Chrome uzantı kimliği her zaman otuz iki karakterdir
    for (wchar_t karakter : aday)
    {
        if (karakter < L'a' || karakter > L'p') { return L""; } // Uzantı kimlikleri sadece 'a' ile 'p' arası harflerden oluşur (on altılık benzeri ama a-p tabanlı özel bir kodlama)
    }
    return aday;
}

// Bu fonksiyon, verilen bir Chrome Web Mağazası sayfası adresinden uzantıyı indirip yüklemeye çalışır
void BrowserWindow::EklentiMagazaAdresindenYukle(int sekmeId, const std::wstring& magazaUrl)
{
    std::wstring eklentiId = MagazaAdresindenKimlikCikar(magazaUrl);
    if (eklentiId.empty())
    {
        SekmeyeMesajGonder(sekmeId, L"{\"type\":\"store_extension_result\",\"success\":false,\"error\":\"Adresten geçerli bir uzantı kimliği çıkarılamadı. Adresin sonunda otuz iki karakterlik bir kimlik olmalı.\"}");
        return;
    }

    HWND pencere = m_pencere;
    std::shared_ptr<bool> gecerlilikBayragi = m_pencereGecerliMi;
    std::thread([pencere, sekmeId, eklentiId, gecerlilikBayragi]()
    {
        MagazaEklentiSonucu* sonuc = new MagazaEklentiSonucu();
        sonuc->sekmeId = sekmeId;

        // Google'ın resmi CRX indirme uç noktası; "response=redirect" parametresi, sunucunun gerçek dosyayı barındıran bir CDN adresine 302 ile yönlendirmesini sağlar, WinHttp bunu otomatik takip eder
        std::wstring istekYolu = L"/service/update2/crx?response=redirect&acceptformat=crx3&prodversion=120.0.0.0&x=id%3D" + eklentiId + L"%26installsource%3Dondemand%26uc";
        std::vector<char> crxBaytlari = UzakHttpsGetIkili(L"clients2.google.com", istekYolu, 15000);

        if (!*gecerlilikBayragi) { delete sonuc; return; } // Pencere bu sırada kapatılmaya başladıysa, daha fazla işlem yapmaya gerek yoktur

        if (crxBaytlari.size() < 100) // Gerçek bir uzantı paketi, bu kadar küçük olamaz; muhtemelen bir hata sayfası indirildi
        {
            sonuc->hataMesaji = L"Uzantı indirilemedi (ağ hatası veya bu kimlikte bir uzantı yok).";
            PostMessageW(pencere, MagazaEklentiSonucuMesaji, 0, reinterpret_cast<LPARAM>(sonuc));
            return;
        }

        size_t zipBaslangici = CrxIcindenZipBaslangiciniBul(crxBaytlari);
        if (zipBaslangici == SIZE_MAX)
        {
            sonuc->hataMesaji = L"İndirilen dosya tanınan bir CRX paketi değil.";
            PostMessageW(pencere, MagazaEklentiSonucuMesaji, 0, reinterpret_cast<LPARAM>(sonuc));
            return;
        }

        std::wstring eklentilerKlasoru = VeriKlasoruAl() + L"\\eklentiler";
        CreateDirectoryW(eklentilerKlasoru.c_str(), nullptr);
        std::wstring hedefKlasor = eklentilerKlasoru + L"\\" + eklentiId;
        CreateDirectoryW(hedefKlasor.c_str(), nullptr);
        std::wstring geciciZipYolu = eklentilerKlasoru + L"\\" + eklentiId + L".zip";

        HANDLE dosya = CreateFileW(geciciZipYolu.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        bool yazmaBasarili = false;
        if (dosya != INVALID_HANDLE_VALUE)
        {
            DWORD yazilanBayt = 0;
            yazmaBasarili = WriteFile(dosya, crxBaytlari.data() + zipBaslangici, static_cast<DWORD>(crxBaytlari.size() - zipBaslangici), &yazilanBayt, nullptr) != 0;
            CloseHandle(dosya);
        }
        if (!yazmaBasarili)
        {
            sonuc->hataMesaji = L"Geçici paket dosyası diske yazılamadı.";
            PostMessageW(pencere, MagazaEklentiSonucuMesaji, 0, reinterpret_cast<LPARAM>(sonuc));
            return;
        }

        // ZIP paketinin açılması için, Windows'ta varsayılan olarak yüklü gelen "tar.exe" (bsdtar) komut satırı aracı kullanılır, bu araç ZIP formatını da destekler, ekstra bir kütüphane bağımlılığı gerekmez
        std::wstring komutSatiri = L"tar.exe -xf \"" + geciciZipYolu + L"\" -C \"" + hedefKlasor + L"\"";
        STARTUPINFOW baslatmaBilgisi = {};
        baslatmaBilgisi.cb = sizeof(baslatmaBilgisi);
        PROCESS_INFORMATION surecBilgisi = {};
        bool surecBasladiMi = CreateProcessW(nullptr, komutSatiri.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &baslatmaBilgisi, &surecBilgisi) != 0;

        bool acmaBasarili = false;
        if (surecBasladiMi)
        {
            WaitForSingleObject(surecBilgisi.hProcess, 15000);
            DWORD cikisKodu = 1;
            GetExitCodeProcess(surecBilgisi.hProcess, &cikisKodu);
            acmaBasarili = (cikisKodu == 0);
            CloseHandle(surecBilgisi.hProcess);
            CloseHandle(surecBilgisi.hThread);
        }
        DeleteFileW(geciciZipYolu.c_str()); // Geçici ZIP dosyası, başarılı olsa da olmasa da artık gerekmiyor

        if (!*gecerlilikBayragi) { delete sonuc; return; }

        if (!acmaBasarili)
        {
            sonuc->hataMesaji = L"İndirilen paket açılamadı.";
            PostMessageW(pencere, MagazaEklentiSonucuMesaji, 0, reinterpret_cast<LPARAM>(sonuc));
            return;
        }

        // manifest.json dosyasının, açılan klasörün en üst seviyesinde olduğu doğrulanır, aksi halde AddBrowserExtension zaten başarısız olacaktır ama burada daha net bir hata verilebilir
        std::wstring manifestYolu = hedefKlasor + L"\\manifest.json";
        DWORD manifestOzellikleri = GetFileAttributesW(manifestYolu.c_str());
        if (manifestOzellikleri == INVALID_FILE_ATTRIBUTES)
        {
            sonuc->hataMesaji = L"Paket açıldı ama manifest.json bulunamadı, uzantı bu basit yöntemle yüklenemiyor olabilir.";
            PostMessageW(pencere, MagazaEklentiSonucuMesaji, 0, reinterpret_cast<LPARAM>(sonuc));
            return;
        }

        sonuc->basarili = true;
        sonuc->klasorYolu = hedefKlasor;
        PostMessageW(pencere, MagazaEklentiSonucuMesaji, 0, reinterpret_cast<LPARAM>(sonuc));
    }).detach();
}

// Bu fonksiyon, mevcut tüm profillerin adlarını ve aktif profili JSON olarak gönderir
void BrowserWindow::ProfilListesiniGonder(int sekmeId)
{
    std::wstring profillerKlasoru = KokVeriKlasoruAl() + L"\\profiller";

    std::wstring listeMetni = L"[";
    bool ilkMi = true;

    WIN32_FIND_DATAW bulunan;
    HANDLE arama = FindFirstFileW((profillerKlasoru + L"\\*").c_str(), &bulunan);
    if (arama != INVALID_HANDLE_VALUE)
    {
        do
        {
            bool klasorMu = (bulunan.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            bool noktaKlasoruMu = (wcscmp(bulunan.cFileName, L".") == 0 || wcscmp(bulunan.cFileName, L"..") == 0);
            if (klasorMu && !noktaKlasoruMu)
            {
                if (!ilkMi) { listeMetni += L","; }
                listeMetni += L"\"" + JsonIcinKacisla(bulunan.cFileName) + L"\"";
                ilkMi = false;
            }
        } while (FindNextFileW(arama, &bulunan));
        FindClose(arama);
    }
    listeMetni += L"]";

    std::wstring mesaj = L"{\"type\":\"profiles\",\"profiles\":" + listeMetni + L",\"active\":\"" + JsonIcinKacisla(AktifProfilAdiReferansi()) + L"\"}";
    SekmeyeMesajGonder(sekmeId, mesaj);
}

// Bu fonksiyon, programı verilen profille yeniden başlatır ve bu süreci kapatır
void BrowserWindow::ProfileGecisYapVeYenidenBaslat(const std::wstring& profilAdiHam)
{
    // Profil adı, doğrudan bir klasör adı olarak kullanılacağı için, yol gezinme veya özel karakter riskine karşı sadece harf/rakam/alt çizgi/tire içermesine izin verilir
    std::wstring profilAdi;
    for (wchar_t karakter : profilAdiHam)
    {
        if (iswalnum(karakter) || karakter == L'_' || karakter == L'-') { profilAdi += karakter; }
    }

    wchar_t suankiExeYolu[MAX_PATH];
    GetModuleFileNameW(nullptr, suankiExeYolu, MAX_PATH);

    std::wstring komutSatiri = std::wstring(L"\"") + suankiExeYolu + L"\""; // Yeni sürece verilecek argüman, varsayılan (profilsiz) profile geçiliyorsa boş bırakılır
    if (!profilAdi.empty()) { komutSatiri += L" --profil=" + profilAdi; }

    STARTUPINFOW baslatmaBilgisi = {};
    baslatmaBilgisi.cb = sizeof(baslatmaBilgisi);
    PROCESS_INFORMATION surecBilgisi = {};
    if (CreateProcessW(nullptr, komutSatiri.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &baslatmaBilgisi, &surecBilgisi))
    {
        CloseHandle(surecBilgisi.hProcess);
        CloseHandle(surecBilgisi.hThread);
    }

    PostMessage(m_pencere, WM_CLOSE, 0, 0); // Yeni süreç başlatıldığına göre, bu (eski profildeki) pencerenin normal kapanma süreci başlatılır
}

// Bu fonksiyon, Windows şifresiyle doğrulandıktan sonra verilen profilin klasörünü (ve içindeki tüm WebView2 verisini) kalıcı olarak siler, aktif profil silinemez
void BrowserWindow::ProfiliSil(int sekmeId, const std::wstring& profilAdiHam)
{
    // Profil adı, doğrudan bir klasör adı olarak kullanılacağı için, yol gezinme veya özel karakter riskine karşı sadece harf/rakam/alt çizgi/tire içermesine izin verilir
    std::wstring profilAdi;
    for (wchar_t karakter : profilAdiHam)
    {
        if (iswalnum(karakter) || karakter == L'_' || karakter == L'-') { profilAdi += karakter; }
    }
    if (profilAdi.empty() || profilAdi == AktifProfilAdiReferansi()) { return; } // Boş isim (varsayılan mod) veya şu an aktif olan profil asla silinemez

    if (!Guvenlik::WindowsSifresiDogrula(m_pencere)) { return; } // Silme kalıcı ve geri alınamaz bir işlem olduğu için, devam etmeden önce Windows şifresi istenir

    std::wstring klasorYolu = KokVeriKlasoruAl() + L"\\profiller\\" + profilAdi;

    // SHFileOperationW'nin istediği gibi, yol çift sonlandırıcı null ile biten, değiştirilebilir bir tampon olarak hazırlanır
    std::vector<wchar_t> yolTamponu(klasorYolu.begin(), klasorYolu.end());
    yolTamponu.push_back(L'\0');
    yolTamponu.push_back(L'\0');

    SHFILEOPSTRUCTW silmeIslemi = {};
    silmeIslemi.wFunc = FO_DELETE;
    silmeIslemi.pFrom = yolTamponu.data();
    silmeIslemi.fFlags = FOF_NO_UI | FOF_NOERRORUI | FOF_SILENT; // Geri dönüşüm kutusu sorgusu, ilerleme penceresi ve hata diyalogları olmadan sessizce silinir

    SHFileOperationW(&silmeIslemi); // Klasör ve içindeki tüm WebView2 verisi (çerezler, geçmiş, önbellek) kalıcı olarak silinir

    ProfilListesiniGonder(sekmeId); // Güncellenmiş profil listesi, silme işleminin sonucunu yansıtsın diye hemen yeniden gönderilir
}

// Bu fonksiyon, verilen kimlikli uzantıyı kaldırır veya etkinliğini değiştirir
void BrowserWindow::EklentiyiDegistir(int sekmeId, const std::wstring& eklentiId, const std::wstring& eylem)
{
    ComPtr<ICoreWebView2Profile7> profil = EklentiProfiliniAl(m_toolbarWebView.Get());
    if (profil == nullptr)
    {
        return;
    }

    profil->GetBrowserExtensions(
        Callback<ICoreWebView2ProfileGetBrowserExtensionsCompletedHandler>(
            [this, sekmeId, eklentiId, eylem, gecerlilikBayragi = m_pencereGecerliMi](HRESULT sonuc, ICoreWebView2BrowserExtensionList* liste) -> HRESULT
            {
                if (!*gecerlilikBayragi) { return S_OK; }
                UINT32 sayi = 0;
                if (liste != nullptr) { liste->get_Count(&sayi); }
                for (UINT32 i = 0; i < sayi; i++)
                {
                    ComPtr<ICoreWebView2BrowserExtension> eklenti;
                    liste->GetValueAtIndex(i, &eklenti);
                    if (eklenti == nullptr) { continue; }

                    LPWSTR idPtr = nullptr;
                    eklenti->get_Id(&idPtr);
                    std::wstring bulunanId = idPtr ? idPtr : L"";
                    if (idPtr != nullptr) { CoTaskMemFree(idPtr); }

                    if (bulunanId == eklentiId) // Aranan uzantı bulundu
                    {
                        if (eylem == L"remove") { eklenti->Remove(Callback<ICoreWebView2BrowserExtensionRemoveCompletedHandler>([this, sekmeId, gecerlilikBayragi](HRESULT s) -> HRESULT { if (!*gecerlilikBayragi) { return S_OK; } EklentileriniGonder(sekmeId); return S_OK; }).Get()); }
                        else if (eylem == L"enable") { eklenti->Enable(TRUE, Callback<ICoreWebView2BrowserExtensionEnableCompletedHandler>([this, sekmeId, gecerlilikBayragi](HRESULT s) -> HRESULT { if (!*gecerlilikBayragi) { return S_OK; } EklentileriniGonder(sekmeId); return S_OK; }).Get()); }
                        else if (eylem == L"disable") { eklenti->Enable(FALSE, Callback<ICoreWebView2BrowserExtensionEnableCompletedHandler>([this, sekmeId, gecerlilikBayragi](HRESULT s) -> HRESULT { if (!*gecerlilikBayragi) { return S_OK; } EklentileriniGonder(sekmeId); return S_OK; }).Get()); }
                        break;
                    }
                }
                return S_OK;
            }
        ).Get()
    );
}

// Bu fonksiyon, dahili bir klavye kısayolları yardım sekmesi açar, eğer zaten açıksa var olan sekmeye geçer
void BrowserWindow::KisayollarSekmesiniAc()
{
    if (m_kisayollarSekmeId != -1 && SekmeBul(m_kisayollarSekmeId) != nullptr) // Eğer zaten açık bir kısayollar sekmesi varsa
    {
        SekmeyeGec(m_kisayollarSekmeId); // Var olan sekmeye geçiş yapılır, yenisi açılmaz
        return; // Fonksiyondan çıkılır
    }

    m_kisayollarSekmeId = YeniSekmeOlustur(L"https://tarayici.local/shortcuts.html", true, true); // Yeni, dahili bir kısayollar sekmesi açılır
}

// Bu fonksiyon, yeni bir özel (gizli) sekme açar, bu sekmede gezinme geçmişe kaydedilmez
void BrowserWindow::GizliSekmeAc()
{
    YeniSekmeOlustur(L"https://tarayici.local/newtab.html", true, true, true); // Yeni sekme, hızlı erişim sayfasıyla ve gizli olarak işaretlenmiş şekilde açılır
}

// Bu fonksiyon, bir dosya indirmesi başladığında çağrılır, çalıştırılabilir dosya uzantıları için kullanıcıya bir onay sorusu sorar
void BrowserWindow::IndirmeBasladi(ICoreWebView2DownloadStartingEventArgs* argumanlar)
{
    ComPtr<ICoreWebView2DownloadOperation> indirmeIslemi; // İndirme işlemiyle ilgili ayrıntılı bilgiye erişmek için kullanılacak nesne
    argumanlar->get_DownloadOperation(&indirmeIslemi); // İndirme işlemi nesnesi alınır
    if (indirmeIslemi == nullptr) // Eğer bir sebepten bu nesneye erişilemediyse
    {
        return; // Güvenli tarafta kalıp, indirmenin olduğu gibi (WebView2'nin kendi varsayılan davranışıyla) devam etmesine izin verilir
    }

    LPWSTR hamAdres = nullptr; // İndirilen dosyanın adresini tutacak değişken
    indirmeIslemi->get_Uri(&hamAdres); // İndirilen dosyanın adresi okunur
    std::wstring adres = hamAdres ? hamAdres : L""; // Adres, güvenli bir metin değişkenine aktarılır
    if (hamAdres != nullptr) { CoTaskMemFree(hamAdres); } // Ayrılan bellek serbest bırakılır

    std::wstring uzanti; // Dosyanın uzantısını tutacak metin
    size_t sonNokta = adres.find_last_of(L'.'); // Adresteki en sondaki nokta işaretinin konumu aranır
    size_t sonSoruIsareti = adres.find_last_of(L'?'); // Adresteki sorgu parametrelerinin başladığı yer aranır, uzantı bu kısımdan önce olmalıdır
    if (sonNokta != std::wstring::npos && (sonSoruIsareti == std::wstring::npos || sonNokta < sonSoruIsareti)) // Eğer geçerli bir nokta bulunduysa
    {
        std::wstring hamUzanti = adres.substr(sonNokta + 1, (sonSoruIsareti == std::wstring::npos ? adres.size() : sonSoruIsareti) - sonNokta - 1); // Noktadan sonraki, sorgu parametrelerinden önceki kısım uzantı olarak alınır
        for (wchar_t karakter : hamUzanti) { uzanti += towlower(karakter); } // Uzantı, karşılaştırma kolay olsun diye küçük harfe çevrilir
    }

    if (Ayarlar::Oku().indirmeKonumunuHerSeferSor) // Kullanıcı, her indirmede kaydedilecek klasörün sorulmasını istemişse
    {
        LPWSTR varsayilanYolPtr = nullptr; // WebView2'nin planladığı varsayılan tam yol, sadece dosya adını ayıklamak için okunur
        indirmeIslemi->get_ResultFilePath(&varsayilanYolPtr);
        std::wstring varsayilanYol = varsayilanYolPtr ? varsayilanYolPtr : L"";
        if (varsayilanYolPtr != nullptr) { CoTaskMemFree(varsayilanYolPtr); }
        size_t sonTersBoluVarsayilan = varsayilanYol.find_last_of(L'\\');
        std::wstring varsayilanDosyaAdi = (sonTersBoluVarsayilan == std::wstring::npos) ? varsayilanYol : varsayilanYol.substr(sonTersBoluVarsayilan + 1);

        wchar_t secilenKlasor[MAX_PATH] = {};
        BROWSEINFOW gozatBilgisi = {}; // Windows'un standart "Klasör Seç" penceresini açmak için gereken ayarlar, eklenti klasörü seçiminde kullanılan aynı desen
        gozatBilgisi.hwndOwner = m_pencere;
        gozatBilgisi.lpszTitle = L"İndirilen dosyanın kaydedileceği klasörü seçin";
        gozatBilgisi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        LPITEMIDLIST sonuc = SHBrowseForFolderW(&gozatBilgisi);
        if (sonuc != nullptr) // Kullanıcı bir klasör seçtiyse (iptal etmediyse)
        {
            SHGetPathFromIDListW(sonuc, secilenKlasor);
            CoTaskMemFree(sonuc);
            if (!varsayilanDosyaAdi.empty()) // Seçilen klasörle, orijinal dosya adı birleştirilip yeni hedef yol WebView2'ye bildirilir
            {
                std::wstring yeniYol = std::wstring(secilenKlasor) + L"\\" + varsayilanDosyaAdi;
                argumanlar->put_ResultFilePath(yeniYol.c_str());
            }
        }
        // Kullanıcı iptal ettiyse, WebView2'nin planladığı varsayılan konum olduğu gibi kullanılır, indirme iptal edilmez
    }

    const wchar_t* riskliUzantilar[] = { L"exe", L"msi", L"bat", L"cmd", L"ps1", L"scr", L"vbs" }; // Çalıştırıldığında bilgisayara zarar verebilecek, bu yüzden ekstra onay istenen dosya uzantıları
    bool riskliMi = false; // Bu indirmenin riskli bir uzantıya sahip olup olmadığını tutacak değişken
    for (const wchar_t* riskli : riskliUzantilar) // Her riskli uzantı tek tek kontrol edilir
    {
        if (uzanti == riskli) // Eğer indirilen dosyanın uzantısı bu riskli uzantılardan biriyse
        {
            riskliMi = true; // Riskli olduğu işaretlenir
            break; // Daha fazla kontrol etmeye gerek yoktur
        }
    }

    if (riskliMi) // Eğer indirilen dosya riskli bir uzantıya sahipse
    {
        std::wstring soru = L"Bu indirdiğiniz dosya (." + uzanti + L") çalıştırılabilir bir dosya türüdür.\n\nGüvendiğiniz bir kaynaktan değilse indirmeyi iptal etmenizi öneririz.\n\nİndirmeye devam edilsin mi?"; // Kullanıcıya sorulacak soru hazırlanır
        int sonuc = MessageBoxW(m_pencere, soru.c_str(), L"İndirme Onayı", MB_YESNO | MB_ICONWARNING); // Kullanıcıya bir evet/hayır penceresi gösterilir
        if (sonuc == IDNO) // Eğer kullanıcı indirmeyi istemediyse
        {
            argumanlar->put_Cancel(TRUE); // İndirme iptal edilir
        }
    }

    // Riskli olmayan dosyalar için (resim, belge, pdf gibi) hiçbir ekstra onay istenmez, WebView2'nin kendi indirme ilerleme göstergesi normal şekilde çalışmaya devam eder, Handled alanına hiç dokunulmadığı için Windows'un kendi güvenlik işaretlemesi (Mark of the Web) de bozulmaz

    BOOL iptalEdildiMi = FALSE; // Yukarıdaki riskli dosya onayında kullanıcı "Hayır" demiş olabilir, bu durumda indirmeyi listeye hiç eklemeye gerek yoktur
    argumanlar->get_Cancel(&iptalEdildiMi);
    if (iptalEdildiMi)
    {
        return;
    }

    int indirmeId = m_sonrakiIndirmeId++; // Bu indirmeye, diğer indirmelerden ayırt edilebilmesi için yeni bir kimlik numarası verilir

    IndirmeBilgisi bilgi; // Bu indirmeyi takip etmek için gereken bilgiler hazırlanır
    bilgi.id = indirmeId;
    bilgi.islem = indirmeIslemi;

    LPWSTR hedefYolPtr = nullptr; // İndirmenin diskteki (planlanan veya tamamlanmış) tam yolu okunur
    indirmeIslemi->get_ResultFilePath(&hedefYolPtr);
    bilgi.hedefYol = hedefYolPtr ? hedefYolPtr : L"";
    if (hedefYolPtr != nullptr) { CoTaskMemFree(hedefYolPtr); }

    size_t sonTersBolu = bilgi.hedefYol.find_last_of(L'\\'); // Tam yoldan, sadece dosya adını ayıklamak için son ters bölü işaretinin konumu aranır
    bilgi.dosyaAdi = (sonTersBolu == std::wstring::npos) ? bilgi.hedefYol : bilgi.hedefYol.substr(sonTersBolu + 1);

    m_indirmeler.push_back(bilgi); // İndirme, takip edilenler listesine eklenir

    const size_t azamiIndirmeKaydiSayisi = 50; // Liste sonsuza kadar büyümesin diye (ve her kayıt kendi COM nesnesini canlı tuttuğu için RAM boşa harcanmasın diye) üst sınır belirlenir
    if (m_indirmeler.size() > azamiIndirmeKaydiSayisi) // Eğer sınır aşıldıysa
    {
        for (size_t i = 0; i < m_indirmeler.size() && m_indirmeler.size() > azamiIndirmeKaydiSayisi; ) // En eski kayıtlardan başlanarak, hâlâ devam etmekte olan indirmeler ASLA silinmez, sadece tamamlanmış/kesilmiş eski kayıtlar silinir
        {
            COREWEBVIEW2_DOWNLOAD_STATE eskiKaydinDurumu = COREWEBVIEW2_DOWNLOAD_STATE_IN_PROGRESS;
            if (m_indirmeler[i].islem != nullptr) { m_indirmeler[i].islem->get_State(&eskiKaydinDurumu); }
            if (eskiKaydinDurumu != COREWEBVIEW2_DOWNLOAD_STATE_IN_PROGRESS) { m_indirmeler.erase(m_indirmeler.begin() + i); } // Devam etmiyorsa silinir, liste bir öğe kısalır, aynı konumdan devam edilir
            else { i++; } // Devam ediyorsa dokunulmaz, bir sonraki kayda geçilir
        }
    }

    // İndirmenin ilerlemesi her değiştiğinde, güncel liste Ayarlar sekmesindeki İndirmeler paneline gönderilir
    indirmeIslemi->add_BytesReceivedChanged(
        Callback<ICoreWebView2BytesReceivedChangedEventHandler>(
            [this](ICoreWebView2DownloadOperation* gonderen, IUnknown* argumanlar) -> HRESULT
            {
                IndirmeListesiniGonder();
                return S_OK;
            }
        ).Get(),
        nullptr
    );

    // İndirmenin durumu (devam ediyor/tamamlandı/kesildi) her değiştiğinde, güncel liste gönderilir, tamamlandıysa ayrıca bir bildirim gösterilir
    indirmeIslemi->add_StateChanged(
        Callback<ICoreWebView2StateChangedEventHandler>(
            [this, indirmeId](ICoreWebView2DownloadOperation* gonderen, IUnknown* argumanlar) -> HRESULT
            {
                COREWEBVIEW2_DOWNLOAD_STATE durum;
                gonderen->get_State(&durum);
                if (durum == COREWEBVIEW2_DOWNLOAD_STATE_COMPLETED) // İndirme başarıyla tamamlandıysa
                {
                    for (IndirmeBilgisi& kayit : m_indirmeler) // Bu indirmenin kaydı bulunur
                    {
                        if (kayit.id == indirmeId && !kayit.tamamlanmaBildirimiGosterildi) // Bildirim, aynı indirme için yanlışlıkla birden fazla kez gösterilmesin diye bir kere gösterildikten sonra bir daha gösterilmez
                        {
                            kayit.tamamlanmaBildirimiGosterildi = true;
                            IndirmeTamamlandiBildirimiGoster(kayit.dosyaAdi);
                            break;
                        }
                    }
                }
                IndirmeListesiniGonder();
                return S_OK;
            }
        ).Get(),
        nullptr
    );

    IndirmeListesiniGonder(); // İndirme başladığı anda da liste bir kez gönderilir, kullanıcı İndirmeler panelini zaten açıksa anında görür
}

// Bu fonksiyon, güncel indirme listesini (ilerleme, durum dahil) Ayarlar sekmesindeki İndirmeler paneline gönderir
void BrowserWindow::IndirmeListesiniGonder()
{
    std::wstring girdilerMetni = L"["; // İndirme listesi, bir JSON dizisi olarak hazırlanır
    for (size_t i = 0; i < m_indirmeler.size(); i++)
    {
        const IndirmeBilgisi& kayit = m_indirmeler[i];
        if (kayit.islem == nullptr) { continue; }

        COREWEBVIEW2_DOWNLOAD_STATE durum = COREWEBVIEW2_DOWNLOAD_STATE_IN_PROGRESS;
        kayit.islem->get_State(&durum);
        INT64 alinanBayt = 0, toplamBayt = 0;
        kayit.islem->get_BytesReceived(&alinanBayt);
        kayit.islem->get_TotalBytesToReceive(&toplamBayt);
        BOOL devamEdebilirMi = FALSE;
        kayit.islem->get_CanResume(&devamEdebilirMi);

        const wchar_t* durumMetni = (durum == COREWEBVIEW2_DOWNLOAD_STATE_COMPLETED) ? L"tamamlandi"
            : (durum == COREWEBVIEW2_DOWNLOAD_STATE_INTERRUPTED) ? (devamEdebilirMi ? L"duraklatildi" : L"kesildi")
            : L"indiriliyor";

        if (i > 0) { girdilerMetni += L","; }
        girdilerMetni += L"{\"id\":" + std::to_wstring(kayit.id) + L",\"name\":\"" + JsonIcinKacisla(kayit.dosyaAdi) + L"\",\"state\":\"" + durumMetni + L"\",\"received\":" + std::to_wstring(alinanBayt) + L",\"total\":" + std::to_wstring(toplamBayt) + L"}";
    }
    girdilerMetni += L"]";

    for (std::unique_ptr<SekmeBilgisi>& sekme : m_sekmeler) // Ayarlar sekmesi her zaman açık olmayabileceği (veya kimliği elde kolayca bulunamayabileceği) için, dahili her sekmeye gönderilir, sadece gerçekten dinleyen (İndirmeler panelindeki) sayfa bunu kullanır
    {
        if (sekme->dahiliSayfaMi && sekme->webView != nullptr)
        {
            sekme->webView->PostWebMessageAsString((L"{\"type\":\"downloads\",\"entries\":" + girdilerMetni + L"}").c_str());
        }
    }
}

// Bu fonksiyon, verilen kimlik numaralı indirmeyi duraklatır, devam ettirir veya iptal eder
void BrowserWindow::IndirmeEylemiUygula(int indirmeId, const std::wstring& eylem)
{
    for (IndirmeBilgisi& kayit : m_indirmeler)
    {
        if (kayit.id == indirmeId && kayit.islem != nullptr)
        {
            if (eylem == L"pause") { kayit.islem->Pause(); }
            else if (eylem == L"resume") { kayit.islem->Resume(); }
            else if (eylem == L"cancel") { kayit.islem->Cancel(); }
            IndirmeListesiniGonder();
            return;
        }
    }
}

// Bu fonksiyon, verilen kimlik numaralı indirmenin dosyasını, Windows Gezgini'nde seçili olarak gösterir
void BrowserWindow::IndirmeyiKlasordeGoster(int indirmeId)
{
    for (const IndirmeBilgisi& kayit : m_indirmeler)
    {
        if (kayit.id == indirmeId && !kayit.hedefYol.empty())
        {
            std::wstring parametre = L"/select,\"" + kayit.hedefYol + L"\""; // Gezgin, bu parametreyle açılan klasörde, belirtilen dosyayı seçili olarak gösterir
            ShellExecuteW(m_pencere, L"open", L"explorer.exe", parametre.c_str(), nullptr, SW_SHOWNORMAL);
            return;
        }
    }
}

// Bu fonksiyon, programın bildirim alanı simgesini oluşturur, bu simge görev çubuğunda normalde görünmez, sadece NIM_MODIFY ile bir bildirim balonu göstermek için bir "kimlik" olarak var olur
void BrowserWindow::BildirimSimgesiOlustur()
{
    NOTIFYICONDATA bildirimVerisi = {};
    bildirimVerisi.cbSize = sizeof(NOTIFYICONDATA);
    bildirimVerisi.hWnd = m_pencere;
    bildirimVerisi.uID = 1;
    bildirimVerisi.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    bildirimVerisi.uCallbackMessage = BildirimSimgesiMesaji;
    bildirimVerisi.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON)); // Programın gömülü simgesi kullanılır
    wcscpy_s(bildirimVerisi.szTip, L"Yuinn");
    Shell_NotifyIcon(NIM_ADD, &bildirimVerisi);
}

// Bu fonksiyon, bildirim alanında, verilen dosya adıyla bir "indirme tamamlandı" balonu gösterir
void BrowserWindow::IndirmeTamamlandiBildirimiGoster(const std::wstring& dosyaAdi)
{
    NOTIFYICONDATA bildirimVerisi = {};
    bildirimVerisi.cbSize = sizeof(NOTIFYICONDATA);
    bildirimVerisi.hWnd = m_pencere;
    bildirimVerisi.uID = 1;
    bildirimVerisi.uFlags = NIF_INFO;
    bildirimVerisi.dwInfoFlags = NIIF_INFO;
    wcscpy_s(bildirimVerisi.szInfoTitle, L"İndirme tamamlandı");
    wcsncpy_s(bildirimVerisi.szInfo, dosyaAdi.c_str(), _TRUNCATE); // Dosya adı, bildirim balonunun ayırdığı sabit alana (en fazla 255 karakter) sığacak şekilde kopyalanır, taşarsa kırpılır
    Shell_NotifyIcon(NIM_MODIFY, &bildirimVerisi);
}

// Bu fonksiyon, uzun süredir aktif olmayan sekmeleri bulup uykuya alır
void BrowserWindow::UykudaSekmeleriKontrolEt()
{
    AyarVerisi ayarlar = Ayarlar::Oku();
    int ayarlananDakika = ayarlar.uykuSekmeEsigiDakika; // Kullanıcının Ayarlar'dan seçtiği eşik, 0 ise uykuya alma tamamen kapalıdır
    if (ayarlananDakika <= 0) { return; } // Kullanıcı bu özelliği kapattıysa, hiçbir sekme kontrol edilmez
    if (ayarlar.enerjiTasarrufuModu) { ayarlananDakika = max(1, ayarlananDakika / 2); } // Enerji tasarrufu modu açıkken, sekmeler yarı sürede (en az bir dakika) uykuya alınır, RAM/CPU daha agresif şekilde korunur
    const ULONGLONG uykuEsigiMs = static_cast<ULONGLONG>(ayarlananDakika) * 60ULL * 1000ULL; // Kullanıcının seçtiği dakika, milisaniyeye çevrilir, TrySuspend tersine çevrilebilir ve ucuz olduğu için varsayılan düşük tutulur, bu sayede RAM daha hızlı geri alınır
    ULONGLONG suankiZaman = GetTickCount64(); // Şu anki an, karşılaştırma için alınır

    for (std::unique_ptr<SekmeBilgisi>& sekme : m_sekmeler) // Açık olan her sekme sırayla kontrol edilir
    {
        bool uykuyaAdayMi = (sekme->id != m_aktifSekmeId) && !sekme->dahiliSayfaMi && !sekme->askidaMi && sekme->denetleyici != nullptr; // Sadece aktif olmayan, dahili olmayan, zaten uykuda olmayan ve denetleyicisi hazır olan (hâlâ "Yükleniyor..." durumunda olmayan) sekmeler değerlendirilir
        if (uykuyaAdayMi && (suankiZaman - sekme->sonAktifZaman) > uykuEsigiMs) // Eğer bu sekme yeterince uzun süredir hareketsizse
        {
            int sekmeId = sekme->id; // Geri çağırma içinde kullanılmak üzere kimlik numarası bir yerel değişkene kopyalanır
            sekme->webViewGenisletilmis->TrySuspend(
                Callback<ICoreWebView2TrySuspendCompletedHandler>(
                    [this, sekmeId](HRESULT sonuc, BOOL basariliMi) -> HRESULT
                    {
                        SekmeBilgisi* guncelSekme = SekmeBul(sekmeId); // Askıya alma tamamlandığında, sekmenin hâlâ açık olup olmadığı tekrar kontrol edilir
                        if (guncelSekme != nullptr && basariliMi) // Eğer sekme hâlâ açıksa ve askıya alma başarılıysa
                        {
                            guncelSekme->askidaMi = true; // Sekme artık uykuda olarak işaretlenir
                            SekmeListesiniGonder(); // Sekme şeridinde uyku rozetinin görünmesi için liste tekrar gönderilir
                        }
                        return S_OK; // İşlem başarıyla ele alındı
                    }
                ).Get()
            );
        }
    }
}

// Bu fonksiyon, programın ve ona bağlı tüm WebView2 süreçlerinin topladığı toplam RAM kullanımını megabayt cinsinden hesaplar
DWORD BrowserWindow::ToplamRamKullanimiMB()
{
    std::set<DWORD> ilgiliSurecler; // Toplamda sayılacak, birbirine ebeveyn-çocuk zinciriyle bağlı tüm süreçlerin kimlik numaralarını tutacak küme
    ilgiliSurecler.insert(GetCurrentProcessId()); // Önce kendi sürecimiz (tarayici.exe) kümeye eklenir

    HANDLE anlikGoruntu = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // Şu anda çalışan tüm süreçlerin bir anlık görüntüsü alınır
    if (anlikGoruntu == INVALID_HANDLE_VALUE) // Eğer anlık görüntü alınamadıysa
    {
        return 0; // Hesaplama yapılamadığı için sıfır döndürülür
    }

    // Bu döngü, ebeveyn-çocuk zincirini takip edip tüm bağlı WebView2 süreçlerini (tarayıcı, render, GPU, ağ süreçleri gibi) bulana kadar tekrar tekrar taranır
    bool yeniSurecBulunduMu = true; // Son taramada yeni bir süreç bulunup bulunmadığını tutan değişken
    while (yeniSurecBulunduMu) // Yeni süreç bulunduğu sürece taramaya devam edilir
    {
        yeniSurecBulunduMu = false; // Bu turda henüz yeni bir şey bulunmadı varsayılır

        PROCESSENTRY32W surecBilgisi; // Taranan her sürecin bilgilerini tutacak yapı
        surecBilgisi.dwSize = sizeof(PROCESSENTRY32W); // Yapının boyutu, Windows'un beklediği şekilde önceden doldurulur

        if (Process32FirstW(anlikGoruntu, &surecBilgisi)) // Anlık görüntüdeki ilk süreçten başlanır
        {
            do // Anlık görüntüdeki her süreç için
            {
                bool ebeveyniIlgiliMi = ilgiliSurecler.count(surecBilgisi.th32ParentProcessID) > 0; // Bu sürecin ebeveyni, zaten ilgili süreçler kümesinde mi diye bakılır
                bool kendisiZatenEkliMi = ilgiliSurecler.count(surecBilgisi.th32ProcessID) > 0; // Bu sürecin kendisi zaten kümede mi diye bakılır
                if (ebeveyniIlgiliMi && !kendisiZatenEkliMi) // Eğer ebeveyni ilgiliyse ve kendisi henüz eklenmediyse
                {
                    ilgiliSurecler.insert(surecBilgisi.th32ProcessID); // Bu süreç de kümeye eklenir
                    yeniSurecBulunduMu = true; // Yeni bir şey bulunduğu için, bir tur daha taramak gerekebilir
                }
            } while (Process32NextW(anlikGoruntu, &surecBilgisi)); // Bir sonraki süreç taranır, bitene kadar devam edilir
        }
    }

    CloseHandle(anlikGoruntu); // Anlık görüntü artık kullanılmadığı için kapatılır

    ULONGLONG toplamBayt = 0; // Tüm ilgili süreçlerin bellek kullanımının toplanacağı değişken
    for (DWORD surecId : ilgiliSurecler) // Bulunan her ilgili süreç için
    {
        HANDLE surecTanitici = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, surecId); // Sürecin bellek bilgisini okuyabilmek için bir tanıtıcı açılır
        if (surecTanitici != nullptr) // Eğer süreç başarıyla açılabildiyse
        {
            PROCESS_MEMORY_COUNTERS bellekBilgisi; // Sürecin bellek kullanım bilgilerini tutacak yapı
            if (K32GetProcessMemoryInfo(surecTanitici, &bellekBilgisi, sizeof(bellekBilgisi))) // Sürecin güncel bellek kullanımı okunur
            {
                toplamBayt += bellekBilgisi.WorkingSetSize; // Sürecin kullandığı fiziksel bellek miktarı, toplam değere eklenir
            }
            CloseHandle(surecTanitici); // Süreç tanıtıcısı artık kullanılmadığı için kapatılır
        }
    }

    return static_cast<DWORD>(toplamBayt / (1024 * 1024)); // Toplam bayt miktarı, megabayta çevrilip döndürülür
}
