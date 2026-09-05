# STM32 E-Bike Motor Kontrol Ünitesi (ECU)

Bu proje, bir çamaşır makinesi motorunun geri dönüştürülerek STM32 (ARM Cortex-M4) mikrodenetleyicisi üzerinden kapalı çevrim (closed-loop) hız kontrolünün yapıldığı bir E-Bike sürücü (ECU) prototipidir. 

Proje, tüm işlemleri `main.c` içine yığmak yerine profesyonel gömülü sistem mimarisine uygun olarak **modüler sürücü (driver) kütüphaneleri** ile geliştirilmiştir.

## ⚙️ Donanım Mimarisi
* **MCU:** STM32 (ARM Cortex-M4)
* **Güç Katı:** IRLZ44N MOSFET (PWM Sürüşü) ve 2 Kanallı Röle Modülü (İleri/Geri Yön Koruması)
* **Hız Geri Beslemesi (Feedback):** Tako jeneratör ve LM393 Hız Sensörü Modülü
* **Arayüz:** I2C 16x2 LCD Ekran ve UART Terminal
* **Girişler:** Analog Potansiyometre (Gaz kolu) ve Donanımsal Buton

## 🚀 Yazılım ve Algoritma Özellikleri

* **Kapalı Çevrim PID Kontrolü:** Timer Input Capture (IC) donanımı ile sensörden okunan devir (RPM), PID algoritmasıyla işlenmektedir. Motorun kalkış ataletini (static friction) yenmek için sisteme özel **Kickstart (Başlangıç Torku)** ve **Ramp-Rate (İvmelenme)** algoritmaları eklenmiştir.
* **Durum Makinesi (State-Machine):** Sistem gücü aldığında motoru kilitli tutar (Safe State). UART üzerinden çalışan interaktif bir "Kurulum Sihirbazı" ile İleri/Geri ve Otomatik/Manuel parametreleri seçilmeden motor sürüşe kapalıdır.
* **Gelişmiş Güvenlik ve Acil Stop:** 
  * H-Köprüsü/Röle koruması için yön değişimlerinde motor devrinin 50 RPM altına inmesini bekleyen dinamik frenleme.
  * Klavyeden gönderilen anlık 'C' komutu ile PWM çıkışlarını kesen ve PID integralini sıfırlayan **Global Acil Stop** donanımı.
* **Dijital Filtreleme:** Potansiyometreden gelen analog (ADC/DMA) gürültüleri bastırmak için Low-Pass Filter (Alçak Geçiren Filtre) kullanılmış ve pürüzsüz bir gaz tepkimesi elde edilmiştir.
* **Modüler Yapı:** UART Dairesel Tampon (Circular Buffer), ADC/DMA ve LCD kontrolleri bağımsız `.c` ve `.h` dosyalarında modüler olarak kurgulanmıştır.

## 🛠️ Geliştirme Ortamı
* **IDE:** STM32CubeIDE
* **Konfigürasyon Aracı:** STM32CubeMX (Projeye ait `.ioc` dosyası repo içinde mevcuttur)
* **Dil:** C (HAL Library)

* <img width="1916" height="987" alt="Screenshot 2026-09-05 161359" src="https://github.com/user-attachments/assets/6b7bfa0b-d782-44c3-8a72-8cdb61b885f1" />
