// Bu dosya, çalışan .exe'nin kendi gömülü sürüm bilgisini okuyan fonksiyonun bildirimini içerir.
// Bu fonksiyonun döndürdüğü değer, settings.json içindeki kullanıcı tarafından erişilebilir ayarlardan değil, doğrudan .exe'nin ikili dosyasına (tarayici.rc'deki VERSIONINFO bloğundan) gömülmüş sürüm bilgisinden okunur; bu sayede kullanıcı settings.json'u elle düzenleyip onay ekranını atlatmaya çalışsa bile, "güncel sürüm" karşılaştırması her zaman gerçek .exe sürümüne göre yapılır.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <string> // Metin (std::wstring) türünü kullanmak için eklenir

namespace ProgramSurumu // Bu isim alanı, sürüm okuma fonksiyonunun başka isimlerle çakışmaması için kullanılır
{
    // Bu fonksiyon, çalışan .exe'nin kendi PE kaynağına gömülü ProductVersion metnini okuyup döndürür, örneğin "0.4.0"; okuma herhangi bir nedenle başarısız olursa "0.0.0" döner, bu değer hiçbir zaman gerçek bir kullanıcı onayı sürümüyle eşleşmeyeceği için onay ekranının yanlışlıkla atlanmasını önler
    std::wstring Al();
}
