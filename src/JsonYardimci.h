// Bu dosya, programın farklı yerlerinde (Storage.cpp, Logger.cpp, BrowserWindow.cpp) tekrar tekrar ihtiyaç duyulan, bir metni JSON içine güvenle yazılabilecek hale getiren küçük yardımcı fonksiyonu içerir.
// Bu fonksiyon başlık dosyasının içinde satır içi (inline) olarak tanımlandığı için, bu dosyayı ekleyen her .cpp dosyası kendi kopyasını kullanır, ayrı bir .cpp dosyasına ihtiyaç yoktur.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <string> // Metin (std::wstring) türünü kullanmak için eklenir

// Bu fonksiyon, verilen bir metni, bir JSON metin değerinin içine güvenle yazılabilecek hale getirir, yani tırnak işareti, ters taksim ve satır sonu gibi özel karakterleri kaçış karakteriyle işaretler
inline std::wstring JsonIcinKacisla(const std::wstring& metin)
{
    std::wstring sonuc; // Kaçışlanmış sonucu biriktirecek boş bir metin oluşturulur
    for (wchar_t karakter : metin) // Verilen metindeki her karakter sırayla gezilir
    {
        if (karakter == L'"') { sonuc += L"\\\""; } // Çift tırnak, başına ters taksim eklenerek kaçışlanır
        else if (karakter == L'\\') { sonuc += L"\\\\"; } // Ters taksim, başına bir ters taksim daha eklenerek kaçışlanır
        else if (karakter == L'\n') { sonuc += L"\\n"; } // Yeni satır, JSON'un anladığı kaçış dizisine çevrilir
        else if (karakter == L'\r') { sonuc += L"\\r"; } // Satır başı, JSON'un anladığı kaçış dizisine çevrilir
        else if (karakter == L'\t') { sonuc += L"\\t"; } // Sekme, JSON'un anladığı kaçış dizisine çevrilir
        else if (karakter < 0x20) // Yukarıda ele alınmamış, görünmeyen başka bir kontrol karakteriyse
        {
            wchar_t tamponMetin[8]; // Karakterin kaçışlanmış halini yazmak için küçük bir tampon oluşturulur
            swprintf_s(tamponMetin, L"\\u%04x", static_cast<unsigned int>(karakter)); // Karakter, JSON'un dört haneli onaltılık kaçış biçiminde yazılır
            sonuc += tamponMetin; // Kaçışlanmış karakter, sonuca eklenir
        }
        else { sonuc += karakter; } // Diğer tüm normal karakterler, hiçbir değişiklik yapılmadan sonuca eklenir
    }
    return sonuc; // Tamamen kaçışlanmış metin döndürülür
}

// Bu fonksiyon, bir JSON metin değerinin içindeki kaçışlanmış karakterleri, orijinal haline geri çevirir, yani JsonIcinKacisla fonksiyonunun tam tersini yapar
inline std::wstring JsonKacisiCoz(const std::wstring& metin)
{
    std::wstring sonuc; // Çözülmüş sonucu biriktirecek boş bir metin oluşturulur
    for (size_t i = 0; i < metin.size(); i++) // Verilen metindeki her karakter sırayla gezilir
    {
        if (metin[i] == L'\\' && i + 1 < metin.size()) // Eğer karakter bir ters taksımsa ve metnin sonuna gelinmediyse, bu bir kaçış dizisinin başlangıcı olabilir
        {
            wchar_t sonrakiKarakter = metin[i + 1]; // Ters taksımdan sonraki karaktere bakılır
            if (sonrakiKarakter == L'"') { sonuc += L'"'; i++; } // Kaçışlanmış tırnak, düz tırnağa çevrilir
            else if (sonrakiKarakter == L'\\') { sonuc += L'\\'; i++; } // Kaçışlanmış ters taksım, düz ters taksıma çevrilir
            else if (sonrakiKarakter == L'n') { sonuc += L'\n'; i++; } // Kaçışlanmış yeni satır, gerçek yeni satıra çevrilir
            else if (sonrakiKarakter == L'r') { sonuc += L'\r'; i++; } // Kaçışlanmış satır başı, gerçek satır başına çevrilir
            else if (sonrakiKarakter == L't') { sonuc += L'\t'; i++; } // Kaçışlanmış sekme, gerçek sekmeye çevrilir
            else if (sonrakiKarakter == L'u' && i + 5 < metin.size()) // Eğer dört haneli onaltılık bir kaçış diziyse
            {
                std::wstring onaltilikMetin = metin.substr(i + 2, 4); // Kaçış dizisinin dört haneli kısmı alınır
                wchar_t kod = static_cast<wchar_t>(wcstol(onaltilikMetin.c_str(), nullptr, 16)); // Onaltılık metin, sayısal bir karakter koduna çevrilir
                sonuc += kod; // Çözülen karakter, sonuca eklenir
                i += 5; // Kaçış dizisinin tamamı atlanır
            }
            else { sonuc += sonrakiKarakter; i++; } // Tanınmayan bir kaçış dizisiyse, kaçışlanan karakter olduğu gibi eklenir
        }
        else // Kaçış dizisi olmayan normal bir karakterse
        {
            sonuc += metin[i]; // Karakter, olduğu gibi sonuca eklenir
        }
    }
    return sonuc; // Tamamen çözülmüş metin döndürülür
}

// Bu fonksiyon, bir JSON nesnesi metninin belirli bir anahtarının (alan adının) metin değerini bulup döndürür, örneğin "adres" anahtarının değerini bulmak için kullanılır, sadece düz metin alanları için çalışır
inline std::wstring JsonAlaniBul(const std::wstring& nesneMetni, const std::wstring& anahtar)
{
    std::wstring anahtarDeseni = L"\"" + anahtar + L"\""; // Aranacak, sadece tırnaklı anahtar adından oluşan desen
    size_t konum = nesneMetni.find(anahtarDeseni); // Bu desenin nesne metni içindeki konumu aranır
    if (konum == std::wstring::npos) // Eğer bu anahtar nesnede hiç bulunamadıysa
    {
        return L""; // Boş bir metin döndürülür, çağıran taraf bunu eksik veri olarak yorumlayabilir
    }
    konum += anahtarDeseni.size(); // Arama konumu, anahtarın hemen bittiği yere ilerletilir

    // Bizim kendi yazdığımız JSON hiçbir boşluk içermez, ama WebView2'nin uzaktan hata ayıklama sunucusu gibi dış kaynaklardan gelen JSON metinleri, iki nokta üst üsteden sonra bir boşluk bırakabilir (örneğin "url": "..." gibi)
    // Bu yüzden anahtar ile değer arasında, iki nokta üst üstenin her iki yanında da olası boşluk/satır sonu karakterleri atlanır, bu sayede fonksiyon hem bizim sıkışık biçimimizi hem de dışarıdan gelen boşluklu biçimi aynı şekilde okuyabilir
    auto bosluguAtla = [&nesneMetni](size_t baslangicKonumu) {
        while (baslangicKonumu < nesneMetni.size() && (nesneMetni[baslangicKonumu] == L' ' || nesneMetni[baslangicKonumu] == L'\t' || nesneMetni[baslangicKonumu] == L'\n' || nesneMetni[baslangicKonumu] == L'\r')) { baslangicKonumu++; }
        return baslangicKonumu;
    };

    konum = bosluguAtla(konum); // Anahtar ile iki nokta üst üste arasındaki olası boşluklar atlanır
    if (konum >= nesneMetni.size() || nesneMetni[konum] != L':') { return L""; } // İki nokta üst üste bekleniyordu ama bulunamadı, bu anahtar aslında başka bir alanın değerinin içinde geçiyor olabilir
    konum++; // İki nokta üst üste atlanır
    konum = bosluguAtla(konum); // İki nokta üst üste ile değerin açılış tırnağı arasındaki olası boşluklar atlanır
    if (konum >= nesneMetni.size() || nesneMetni[konum] != L'"') { return L""; } // Bu basit fonksiyon sadece metin değerlerini okur, sayısal/boolean/null değerler için kullanılmaz

    size_t baslangic = konum + 1; // Değerin asıl başladığı yer, açılış tırnağının hemen sonrasıdır

    size_t bitis = baslangic; // Değerin biteceği konumu bulmak için bir sayaç başlatılır
    while (bitis < nesneMetni.size()) // Nesne metninin sonuna gelene kadar
    {
        if (nesneMetni[bitis] == L'"' && nesneMetni[bitis - 1] != L'\\') // Eğer kaçışlanmamış bir kapanış tırnağı bulunduysa, değerin bittiği yer burasıdır
        {
            break; // Döngüden çıkılır
        }
        bitis++; // Henüz bulunamadıysa, bir sonraki karaktere geçilir
    }

    return JsonKacisiCoz(nesneMetni.substr(baslangic, bitis - baslangic)); // Bulunan ham değer, kaçış dizilerinden arındırılarak döndürülür
}
