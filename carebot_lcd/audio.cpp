#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <SPIFFS.h>

#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common.h"
#include "audio.h"

// SPIFFS 미디어 경로
static const char* SPIFFS_MEDIA_PATH = "/media";

#if 0
// SPI 설정 정의
const int SD_SCK = 12;   // SPI Clock - 필요에 따라 수정
const int SD_MISO = 11;  // SPI MISO - 필요에 따라 수정
const int SD_MOSI = 13;  // SPI MOSI - 필요에 따라 수정
#endif
const int SD_CS = 10;

// Global variables
QueueHandle_t soundQueue = NULL;
//SemaphoreHandle_t sdMutex = NULL;
SemaphoreHandle_t i2sMutex = NULL;
SemaphoreHandle_t spiffsMutex = NULL;
SemaphoreHandle_t spiMutex = NULL;

volatile bool isAudioPlaying = false;
volatile bool stopRequested = false;
AudioStorageType currentStorageType = STORAGE_RAM;
AudioFile audioFiles[2] = {
    {NULL, 0, false},  // 001.wav
    {NULL, 0, false}   // 002.wav
};

// 오디오 재생 상태
AudioState audioState = {
    .isPlaying = false,
    .currentSound = NONE,
    .repeatMode = REPEAT_NONE,
    .repeatCount = 0,
    .currentRepeat = 0,
    .playbackTaskHandle = NULL,
    .stopRequested = false
};

// Internal variables
static int16_t* audioBuffer = NULL;
static const int BUFFER_SIZE = 4096;

//static i2s_config_t i2s_config;
//static i2s_pin_config_t pin_config;
uint8_t current_volume = 1;
uint8_t volume_value = 1;

static bool spiffs_initialized = false;

// 내부 함수 선언
static int16_t apply_volume(int16_t sample);
static bool initialize_i2s(void);
static bool read_file_from_sd(const char* filepath, uint8_t** buffer, size_t* size);
static bool ensure_media_directory(void);
static bool play_from_ram(int fileIndex, AudioRepeatMode repeatMode, int repeatCount);
static bool play_from_spiffs(const char* filename, AudioRepeatMode repeatMode, int repeatCount);
static bool play_from_sd(const char* filename, AudioRepeatMode repeatMode, int repeatCount);


/**
 * SD 카드용 SPI 초기화
 */
bool spi_init_for_sd() {
    if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "SPI 뮤텍스 획득 실패");
        return false;
    }
    
    logMessage("AUDIO", LOG_LEVEL_INFO, "SD 카드용 SPI 초기화");
    
    // 기존 SPI 종료
    SPI.end();
    delay(100);
    
    // SD 카드용 SPI 설정
    SPI.begin();
    delay(100);
    
    return true;
}

/**
 * SD 카드용 SPI 종료
 */
void spi_deinit_for_sd() {
    logMessage("AUDIO", LOG_LEVEL_INFO, "SD 카드용 SPI 해제");
    
    // SD 카드 해제
    SD.end();
    
    // SPI 버스 해제
    SPI.end();
    delay(100);
    
    // LCD용 SPI 재초기화는 여기서 하지 않음 (LCD 모듈에서 처리)
    
    xSemaphoreGive(spiMutex);
}

/**
 * SPIFFS 초기화 및 미디어 디렉토리 확인
 */
static bool init_spiffs() {
    // 이미 초기화되었으면 다시 하지 않음
    if (spiffs_initialized) {
        return true;
    }
    
    if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "SPIFFS 뮤텍스 획득 실패");
        return false;
    }
    
    // SPIFFS 초기화
    if (!SPIFFS.begin(true)) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "SPIFFS 초기화 실패");
        xSemaphoreGive(spiffsMutex);
        return false;
    }
    
    // SPIFFS 공간 정보 출력
    uint32_t total = SPIFFS.totalBytes();
    uint32_t used = SPIFFS.usedBytes();
    logMessage("AUDIO", LOG_LEVEL_INFO, "SPIFFS 총 용량: %d KB, 사용 중: %d KB, 여유: %d KB", 
              total / 1024, used / 1024, (total - used) / 1024);
    
    // 미디어 디렉토리 생성 (일부 SPIFFS 구현은 mkdir을 지원하지 않을 수 있음)
    if (!SPIFFS.exists(SPIFFS_MEDIA_PATH)) {
        if (SPIFFS.mkdir(SPIFFS_MEDIA_PATH)) {
            logMessage("AUDIO", LOG_LEVEL_INFO, "미디어 디렉토리 생성: %s", SPIFFS_MEDIA_PATH);
        } else {
            logMessage("AUDIO", LOG_LEVEL_WARN, "미디어 디렉토리 생성 실패 (파일 저장은 계속 가능)");
        }
    }
    
    // 초기화 완료 표시
    spiffs_initialized = true;
    
    xSemaphoreGive(spiffsMutex);
    return true;
}

/**
 * 파일이 SPIFFS에 이미 존재하는지 확인하고 유효성 검사
 * @return: 0 = 없음, 1 = 있고 유효함, -1 = 있지만 크기가 0이거나 손상됨
 */
static int check_file_in_spiffs(const char* filename) {
    if (!spiffs_initialized && !init_spiffs()) {
        return 0;
    }
    
    if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        return 0;
    }
    
    int result = 0;
    
    if (SPIFFS.exists(filename)) {
        File file = SPIFFS.open(filename);
        if (file) {
            size_t size = file.size();
            file.close();
            
            if (size > WAV_HEADER_SIZE) { // 최소한 WAV 헤더보다는 커야 함
                result = 1; // 파일이 존재하고 유효함
            } else {
                result = -1; // 파일이 있지만 손상되었거나 비어 있음
            }
        } else {
            result = -1; // 파일이 있지만 열 수 없음
        }
    }
    
    xSemaphoreGive(spiffsMutex);
    return result;
}


/**
 * SD 카드에서 SPIFFS로 오디오 파일 복사
 * 이미 파일이 존재하면 복사하지 않음
 */
bool audio_copy_files_to_spiffs() {
    static bool files_checked = false;
    static bool files_copied = false;
    
    // 이미 파일 복사가 완료되었으면 다시 하지 않음
    if (files_copied) {
        return true;
    }
    
    // SPIFFS 초기화
    if (!init_spiffs()) {
        return false;
    }
    
    // 복사할 파일 목록
    const char* source_files[] = {"/mp3/001.wav", "/mp3/002.wav"};
    const char* dest_files[] = {"/media/001.wav", "/media/002.wav"};
    const int file_count = 2;
    
    // 파일 상태 확인
    if (!files_checked) {
        bool all_files_exist = true;
        
        for (int i = 0; i < file_count; i++) {
            int file_status = check_file_in_spiffs(dest_files[i]);
            
            if (file_status == 1) {
                logMessage("AUDIO", LOG_LEVEL_INFO, "파일 확인: %s (이미 존재함)", dest_files[i]);
            } else {
                all_files_exist = false;
                if (file_status == -1) {
                    // 손상된 파일 삭제
                    if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                        SPIFFS.remove(dest_files[i]);
                        xSemaphoreGive(spiffsMutex);
                        logMessage("AUDIO", LOG_LEVEL_WARN, "손상된 파일 삭제: %s", dest_files[i]);
                    }
                }
            }
        }
        
        files_checked = true;
        
        // 모든 파일이 이미 있고 유효하면 복사 과정 생략
        if (all_files_exist) {
            logMessage("AUDIO", LOG_LEVEL_INFO, "모든 오디오 파일이 이미 SPIFFS에 존재함, 복사 생략");
            files_copied = true;
            return true;
        }
    }
    
    logMessage("AUDIO", LOG_LEVEL_INFO, "SD 카드에서 SPIFFS로 오디오 파일 복사 중");
    
    // SPI 초기화 (SD 카드용)
    if (!spi_init_for_sd()) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "SD 카드용 SPI 초기화 실패");
        return false;
    }
    
    // SD 카드 초기화
    if (!SD.begin(SD_CS)) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "SD 카드 마운트 실패");
        spi_deinit_for_sd();
        return false;
    }
    
    bool success = true;
    
    // SPIFFS 뮤텍스 획득
    if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "SPIFFS 뮤텍스 획득 실패");
        SD.end();
        spi_deinit_for_sd();
        return false;
    }
    
    // 각 파일 복사
    for (int i = 0; i < file_count; i++) {
        // 파일 상태 재확인
        int file_status = check_file_in_spiffs(dest_files[i]);
        
        // 파일이 이미 있고 유효하면 건너뜀
        if (file_status == 1) {
            logMessage("AUDIO", LOG_LEVEL_INFO, "파일 복사 생략: %s (이미 존재함)", dest_files[i]);
            continue;
        }
        
        // 소스 파일 존재 확인
        if (!SD.exists(source_files[i])) {
            logMessage("AUDIO", LOG_LEVEL_ERROR, "소스 파일이 존재하지 않음: %s", source_files[i]);
            success = false;
            continue;
        }
        
        // 소스 파일 열기
        File src_file = SD.open(source_files[i]);
        if (!src_file) {
            logMessage("AUDIO", LOG_LEVEL_ERROR, "소스 파일 열기 실패: %s", source_files[i]);
            success = false;
            continue;
        }
        
        // 파일 크기 확인
        size_t fileSize = src_file.size();
        
        // 대상 파일 생성
        File dest_file = SPIFFS.open(dest_files[i], FILE_WRITE);
        if (!dest_file) {
            logMessage("AUDIO", LOG_LEVEL_ERROR, "대상 파일 생성 실패: %s", dest_files[i]);
            src_file.close();
            success = false;
            continue;
        }
        
        // 복사 시작
        logMessage("AUDIO", LOG_LEVEL_INFO, "복사 중: %s -> %s (%d 바이트)", 
                  source_files[i], dest_files[i], fileSize);
        
        // 청크로 파일 복사
        uint8_t buffer[1024];
        size_t totalBytes = 0;
        
        // 시간 측정 시작
        unsigned long start_time = millis();
        
        while (src_file.available()) {
            size_t bytesRead = src_file.read(buffer, sizeof(buffer));
            if (bytesRead > 0) {
                size_t bytesWritten = dest_file.write(buffer, bytesRead);
                if (bytesWritten != bytesRead) {
                    logMessage("AUDIO", LOG_LEVEL_ERROR, "쓰기 오류: %d != %d", bytesWritten, bytesRead);
                    success = false;
                    break;
                }
                totalBytes += bytesWritten;
                
                // 진행 상황 출력 (20% 단위)
                if (fileSize > 5120 && totalBytes % (fileSize / 5) < 1024) {
                    logMessage("AUDIO", LOG_LEVEL_INFO, "진행: %d%%", (totalBytes * 100) / fileSize);
                }
            }
        }
        
        // 시간 측정 종료
        unsigned long elapsed_time = millis() - start_time;
        
        src_file.close();
        dest_file.close();
        
        if (totalBytes == fileSize) {
            logMessage("AUDIO", LOG_LEVEL_INFO, "복사 완료: %s (%d 바이트, %d ms)", 
                       dest_files[i], totalBytes, elapsed_time);
        } else {
            logMessage("AUDIO", LOG_LEVEL_ERROR, "복사 실패: %s (%d/%d 바이트)", 
                       dest_files[i], totalBytes, fileSize);
            
            // 손상된 파일 삭제
            SPIFFS.remove(dest_files[i]);
            success = false;
        }
    }
    
    // 정리
    xSemaphoreGive(spiffsMutex);
    SD.end();
    spi_deinit_for_sd();
    
    // 성공하면 플래그 설정
    if (success) {
        files_copied = true;
    }
    
    return success;
}

/**
 * 오디오 시스템 초기화
 */
bool audio_init(AudioStorageType storage_type) {
    logMessage("AUDIO", LOG_LEVEL_INFO, "오디오 시스템 초기화 중 (저장 방식: %d)", storage_type);
    
    // 저장 타입 설정
    currentStorageType = storage_type;

    // 오디오 볼륨 설정
    uint8_t saved_volume = settings.volume_value;
    //logMessage("AUDIO", LOG_LEVEL_INFO, "오디오 볼륨 설정 settings.volume_value (%d)", settings.volume_value);
    if (saved_volume < 1 || saved_volume > 10) saved_volume = 5;
    //logMessage("AUDIO", LOG_LEVEL_INFO, "오디오 볼륨 설정 (%d)", saved_volume);
    //current_volume = audio_volume;
    volume_value = saved_volume;
    //logMessage("AUDIO", LOG_LEVEL_INFO, "오디오 볼륨 설정 current_volume (%d)", current_volume);
    logMessage("AUDIO", LOG_LEVEL_INFO, "오디오 볼륨 설정 (%d)", volume_value);
    audio_set_volume(saved_volume);
    //logMessage("AUDIO", LOG_LEVEL_INFO, "오디오 볼륨 설정 current_volume (%d)", current_volume);

    // I2S 설정 초기화
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = true
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = 42,
        .ws_io_num = 2,
        .data_out_num = 41,
        .data_in_num = -1
    };

    // 뮤텍스 생성
    if (i2sMutex == NULL) {
        i2sMutex = xSemaphoreCreateMutex();
    }
    #if 0
    if (sdMutex == NULL) {
        sdMutex = xSemaphoreCreateMutex();
    }
    #endif
    if (spiffsMutex == NULL) {
        spiffsMutex = xSemaphoreCreateMutex();
    }
    if (spiMutex == NULL) {
        spiMutex = xSemaphoreCreateMutex();
    }
    
    // 뮤텍스 생성 확인
    if (i2sMutex == NULL || spiffsMutex == NULL || spiMutex == NULL) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "뮤텍스 생성 실패");
        return false;
    }

    // 사운드 큐가 없으면 생성
    if (soundQueue == NULL) {
        soundQueue = xQueueCreate(5, sizeof(SoundType));
        if (soundQueue == NULL) {
            logMessage("AUDIO", LOG_LEVEL_ERROR, "사운드 큐 생성 실패");
            return false;
        }
    }

    // 오디오 버퍼 할당
    if (audioBuffer == NULL) {
        audioBuffer = (int16_t*)malloc(BUFFER_SIZE);
        if (audioBuffer == NULL) {
            logMessage("AUDIO", LOG_LEVEL_ERROR, "오디오 버퍼 할당 실패");
            return false;
        }
    }

    // I2S 초기화
    if (!initialize_i2s()) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "I2S 초기화 실패");
        return false;
    }

    // RAM 저장 모드이면 파일을 RAM에 로드
    if (currentStorageType == STORAGE_RAM) {
        // SPI 초기화 (SD 카드용)
        if (!spi_init_for_sd()) {
            logMessage("AUDIO", LOG_LEVEL_ERROR, "SD 카드용 SPI 초기화 실패");
            return false;
        }
        
        // 파일 로드
        bool result = audio_load_files_to_ram();
        
        // SPI 해제 (SD 카드 사용 완료)
        spi_deinit_for_sd();
        
        if (!result) {
            logMessage("AUDIO", LOG_LEVEL_WARN, "RAM에 오디오 파일 로드 실패, SD 카드로 폴백");
            currentStorageType = STORAGE_SD_CARD;
        }
    }
    else if (currentStorageType == STORAGE_SPIFFS) {
        // SPIFFS 초기화
        if (!init_spiffs()) {
            logMessage("AUDIO", LOG_LEVEL_WARN, "SPIFFS 초기화 실패, SD 카드로 폴백");
            currentStorageType = STORAGE_SD_CARD;
            return false;
        }
    
        // 파일 유효성 확인만 수행 (실제 복사는 필요할 때만)
        {
            const char* dest_files[] = {"/media/001.wav", "/media/002.wav"};
            bool all_valid = true;
        
            for (int i = 0; i < 2; i++) {
                int status = check_file_in_spiffs(dest_files[i]);
                if (status != 1) {
                    all_valid = false;
                    break;
                }
            }
        
            if (!all_valid) {
                // 백그라운드 태스크에서 파일 복사 수행
                // (오래 걸리는 작업이므로 비동기로 처리)
                xTaskCreate(
                    [](void* parameter) {
                        audio_copy_files_to_spiffs();
                        vTaskDelete(NULL);
                    },
                    "FileCopy",
                    8192,
                    NULL,
                    1,
                    NULL
                );
            }
        }
    }

    // 사운드 플레이어 태스크 시작
    xTaskCreatePinnedToCore(
        soundPlayerTask,
        "SoundPlayer",
        8192,  // 스택 크기
        NULL,
        2,     // 우선순위
        NULL,
        1      // 코어 1
    );

    // 볼륨 설정
    //audio_set_volume(volume_value*0.1);
    
    logMessage("AUDIO", LOG_LEVEL_INFO, "오디오 시스템 초기화 성공 (최종 저장 방식: %d)", currentStorageType);
    return true;
}

/**
 * 오디오 파일을 RAM에 로드
 */
bool audio_load_files_to_ram(void) {
    logMessage("AUDIO", LOG_LEVEL_INFO, "오디오 파일을 RAM에 로드 중");
    
    const char* files[] = {"/mp3/001.wav", "/mp3/002.wav"};
    bool all_success = true;
    
    // SD 카드 초기화
    if (!SD.begin(SD_CS)) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "SD 카드 마운트 실패");
        return false;
    }
    
    for (int i = 0; i < 2; i++) {
        // 이전에 할당된 메모리 해제
        if (audioFiles[i].data != NULL) {
            free(audioFiles[i].data);
            audioFiles[i].data = NULL;
            audioFiles[i].size = 0;
            audioFiles[i].loaded = false;
        }
        
        // 파일을 메모리에 로드
        if (!read_file_from_sd(files[i], &audioFiles[i].data, &audioFiles[i].size)) {
            logMessage("AUDIO", LOG_LEVEL_ERROR, "RAM에 파일 로드 실패: %s", files[i]);
            all_success = false;
        } else {
            audioFiles[i].loaded = true;
            logMessage("AUDIO", LOG_LEVEL_INFO, "파일을 RAM에 성공적으로 로드: %s (크기: %d 바이트)", 
                      files[i], audioFiles[i].size);
        }
    }
    
    // SD 카드 해제
    SD.end();
    
    return all_success;
}

/**
 * SD 카드에서 파일을 메모리로 읽는 헬퍼 함수
 */
static bool read_file_from_sd(const char* filepath, uint8_t** buffer, size_t* size) {
    bool success = false;
    
    // 파일 열기
    File file = SD.open(filepath);
    if (!file) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "파일 열기 실패: %s", filepath);
        return false;
    }
    
    // 파일 크기 가져오기
    size_t fileSize = file.size();
    *size = fileSize;
    
    // 전체 파일을 위한 메모리 할당
    *buffer = (uint8_t*)malloc(fileSize);
    if (*buffer == NULL) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "파일을 위한 메모리 할당 실패: %s (크기: %d 바이트)", 
                  filepath, fileSize);
        file.close();
        return false;
    }
    
    // 파일 읽기
    if (file.read(*buffer, fileSize) == fileSize) {
        success = true;
    } else {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "전체 파일 읽기 실패: %s", filepath);
        free(*buffer);
        *buffer = NULL;
        *size = 0;
    }
    
    // 파일 닫기
    file.close();
    
    return success;
}

/**
 * 특정 파일의 오디오 재생 시작
 */
bool audio_start(const char* filename, AudioRepeatMode repeatMode, int repeatCount) {
    logMessage("AUDIO", LOG_LEVEL_INFO, "파일 오디오 재생 시작: %s", filename);
    
    // 이미 재생 중인 경우 중지
    if (audioState.isPlaying) {
        audio_stop();
        vTaskDelay(pdMS_TO_TICKS(50));  // 정리 시간
    }
    
    // 요청된 파일 확인 (001 또는 002)
    int fileIndex = -1;
    if (strcmp(filename, "001") == 0) {
        fileIndex = 0;
        audioState.currentSound = WATER;
    } else if (strcmp(filename, "002") == 0) {
        fileIndex = 1;
        audioState.currentSound = URINATION;
    } else {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "알 수 없는 파일 요청: %s", filename);
        return false;
    }
    
    // 재생 상태 설정
    audioState.repeatMode = repeatMode;
    audioState.repeatCount = repeatCount;
    audioState.currentRepeat = 0;
    audioState.stopRequested = false;
    audioState.isPlaying = true;
    
    // 태스크 매개변수 구조체 생성 및 초기화
    AudioTaskParams* params = (AudioTaskParams*)malloc(sizeof(AudioTaskParams));
    if (params == NULL) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "태스크 매개변수 메모리 할당 실패");
        audioState.isPlaying = false;
        return false;
    }
    
    params->fileIndex = fileIndex;
    strcpy(params->filename, filename);
    params->repeatMode = repeatMode;
    params->repeatCount = repeatCount;
    
    // 별도 태스크에서 재생 (터치 및 다른 이벤트 차단 방지)
    xTaskCreatePinnedToCore(
        audio_playback_task,
        "AudioPlayback",
        8192,
        (void*)params,  // 매개변수 구조체 전달
        1,              // 낮은 우선순위로 설정
        &audioState.playbackTaskHandle,
        1               // 코어 1
    );
    
    return true;
}

/**
 * 오디오 재생 태스크
 */
void audio_playback_task(void* parameter) {
    // 매개변수 구조체 가져오기
    AudioTaskParams* params = (AudioTaskParams*)parameter;
    
    // 로컬 변수로 복사
    int fileIndex = params->fileIndex;
    AudioRepeatMode repeatMode = params->repeatMode;
    int repeatCount = params->repeatCount;
    char filename[16];
    strcpy(filename, params->filename);
    
    // 매개변수 구조체 메모리 해제
    free(params);
    
    bool result = false;
    
    // 재생 시작
    do {
        // 반복 횟수 증가
        audioState.currentRepeat++;
        
        // 저장 방식에 따라 재생
        switch (currentStorageType) {
            case STORAGE_RAM:
                if (audioFiles[fileIndex].loaded) {
                    //result = play_from_ram(fileIndex, REPEAT_NONE, 0);
                    result = play_from_ram(fileIndex, repeatMode, repeatCount);
                } else {
                    // RAM에 로드되지 않은 경우 SD 카드에서 재생
                    result = play_from_sd(filename, REPEAT_NONE, 0);
                }
                break;
                
            case STORAGE_SPIFFS: {
                // 파일이 SPIFFS에 존재하는지 확인
                char filepath[64];
                snprintf(filepath, sizeof(filepath), "%s/%s.wav", SPIFFS_MEDIA_PATH, filename);
    
                if (check_file_in_spiffs(filepath) != 1) {
                    // 필요한 파일만 복사 시도
                    if (!audio_copy_files_to_spiffs()) {
                        // 복사 실패 시 SD 카드에서 직접 재생
                        #if 1
                        logMessage("AUDIO", LOG_LEVEL_WARN, 
                                 "SPIFFS에 파일이 없고 복사 실패, SD 카드에서 직접 재생");
                        result = play_from_sd(filename, repeatMode, repeatCount);
                        #endif
                        break;
                    }
                }
    
                // SPIFFS에서 재생
                result = play_from_spiffs(filename, repeatMode, repeatCount);
                break;
            }
                
            case STORAGE_SD_CARD:
            default:
                // SD 카드 모드
                result = play_from_sd(filename, REPEAT_NONE, 0);
                break;
        }
        
        // 중지 요청이 있으면 중단
        if (audioState.stopRequested) {
            break;
        }
        
        // 반복 조건 확인
    } while ((audioState.repeatMode == REPEAT_INFINITE) || 
             (audioState.repeatMode == REPEAT_COUNT && audioState.currentRepeat < audioState.repeatCount));
    
    // 재생 상태 초기화
    audioState.isPlaying = false;
    audioState.currentSound = NONE;
    audioState.playbackTaskHandle = NULL;
    
    // 태스크 삭제
    vTaskDelete(NULL);
}
/**
 * RAM에서 오디오 재생
 */
static bool play_from_ram(int fileIndex, AudioRepeatMode repeatMode, int repeatCount) {
    if (xSemaphoreTake(i2sMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "I2S 뮤텍스 획득 실패");
        return false;
    }
    
    // I2S 초기화
    if (!initialize_i2s()) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "I2S 초기화 실패");
        xSemaphoreGive(i2sMutex);
        return false;
    }
    
    logMessage("AUDIO", LOG_LEVEL_INFO, "RAM에서 재생: 파일 인덱스 %d, 반복 모드 %d, 반복 횟수 %d", 
               fileIndex, repeatMode, repeatCount);
    
    // WAV 헤더 건너뛰기
    uint8_t* audioData = audioFiles[fileIndex].data + WAV_HEADER_SIZE;
    size_t audioDataSize = audioFiles[fileIndex].size - WAV_HEADER_SIZE;
    
    // 반복 카운터 초기화
    int currentRepeatCount = 0;
    bool continuePlayback = true;
    
    // 반복 재생 루프
    while (continuePlayback && !audioState.stopRequested) {
        // 청크 단위로 처리
        size_t bytesProcessed = 0;
        size_t bytesWritten = 0;
        
        while (bytesProcessed < audioDataSize && !audioState.stopRequested) {
            // 청크 크기 결정
            size_t chunkSize = MIN(BUFFER_SIZE, audioDataSize - bytesProcessed);
            
            // 처리 버퍼에 복사
            memcpy(audioBuffer, audioData + bytesProcessed, chunkSize);
            
            // 볼륨 적용
            int samples = chunkSize / 2;
            for (int i = 0; i < samples; i++) {
                audioBuffer[i] = apply_volume(audioBuffer[i]);
            }
            
            // I2S에 쓰기
            i2s_write(I2S_NUM_0, audioBuffer, chunkSize, &bytesWritten, portMAX_DELAY);
            
            // 처리된 바이트 업데이트
            bytesProcessed += chunkSize;
            
            // 태스크 기아 방지를 위한 작은 지연 및 이벤트 처리 허용
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        
        // 반복 모드에 따른 플레이백 결정
        switch (repeatMode) {
            case REPEAT_NONE:
                continuePlayback = false;  // 반복 없음, 한 번만 재생
                break;
                
            case REPEAT_INFINITE:
                continuePlayback = true;   // 무한 반복
                logMessage("AUDIO", LOG_LEVEL_DEBUG, "무한 반복 재생 중: 반복 %d회 완료", currentRepeatCount + 1);
                break;
                
            case REPEAT_COUNT:
                currentRepeatCount++;
                continuePlayback = (currentRepeatCount < repeatCount);  // 지정된 횟수만큼 반복
                logMessage("AUDIO", LOG_LEVEL_DEBUG, "반복 재생 중: %d/%d 회 완료", 
                           currentRepeatCount, repeatCount);
                break;
        }
    }
    
    // 정리
    i2s_zero_dma_buffer(I2S_NUM_0);
    xSemaphoreGive(i2sMutex);
    
    if (audioState.stopRequested) {
        logMessage("AUDIO", LOG_LEVEL_INFO, "오디오 재생이 중지되었습니다");
    } else {
        logMessage("AUDIO", LOG_LEVEL_INFO, "오디오 재생이 완료되었습니다");
    }
    
    return true;
}

/**
 * SD 카드에서 오디오 재생
 */
static bool play_from_sd(const char* filename, AudioRepeatMode repeatMode, int repeatCount) {
    // SPI 초기화 (SD 카드용)
    if (!spi_init_for_sd()) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "SD 카드용 SPI 초기화 실패");
        return false;
    }
    
    // I2S 뮤텍스 획득
    if (xSemaphoreTake(i2sMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "I2S 뮤텍스 획득 실패");
        spi_deinit_for_sd();
        return false;
    }
    
    // I2S 초기화
    if (!initialize_i2s()) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "I2S 초기화 실패");
        xSemaphoreGive(i2sMutex);
        spi_deinit_for_sd();
        return false;
    }
    
    logMessage("AUDIO", LOG_LEVEL_INFO, "SD 카드에서 재생: 파일 %s", filename);
    
    // 경로 구성
    char filepath[32];
    snprintf(filepath, sizeof(filepath), "/mp3/%s.wav", filename);
    
    // SD 카드 초기화
    if (!SD.begin(SD_CS)) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "SD 카드 마운트 실패");
        xSemaphoreGive(i2sMutex);
        spi_deinit_for_sd();
        return false;
    }
    
    // 파일 존재 여부 확인
    if (!SD.exists(filepath)) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "파일이 존재하지 않음: %s", filepath);
        xSemaphoreGive(i2sMutex);
        spi_deinit_for_sd();
        return false;
    }
    
    // 파일 열기
    File file = SD.open(filepath);
    if (!file) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "파일 열기 실패: %s", filepath);
        xSemaphoreGive(i2sMutex);
        spi_deinit_for_sd();
        return false;
    }
    
    // WAV 헤더 건너뛰기
    uint8_t wav_header[WAV_HEADER_SIZE];
    if (file.read(wav_header, WAV_HEADER_SIZE) != WAV_HEADER_SIZE) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "WAV 헤더 읽기 실패: %s", filepath);
        file.close();
        xSemaphoreGive(i2sMutex);
        spi_deinit_for_sd();
        return false;
    }
    
    // 오디오 데이터 재생
    size_t bytesRead = 0;
    size_t bytesWritten = 0;
    
    while (file.available() && !audioState.stopRequested) {
        bytesRead = file.read((uint8_t*)audioBuffer, BUFFER_SIZE);
        if (bytesRead > 0) {
            // 볼륨 적용
            int samples = bytesRead / 2;
            for (int i = 0; i < samples; i++) {
                audioBuffer[i] = apply_volume(audioBuffer[i]);
            }
            
            // I2S에 쓰기
            i2s_write(I2S_NUM_0, audioBuffer, bytesRead, &bytesWritten, portMAX_DELAY);
            
            // 태스크 기아 방지를 위한 작은 지연 및 이벤트 처리 허용
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    
    // 정리
    file.close();
    i2s_zero_dma_buffer(I2S_NUM_0);
    xSemaphoreGive(i2sMutex);
    spi_deinit_for_sd();
    
    return true;
}

/**
 * SPIFFS에서 오디오 재생
 */
static bool play_from_spiffs(const char* filename, AudioRepeatMode repeatMode, int repeatCount) {
    if (xSemaphoreTake(i2sMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "I2S 뮤텍스 획득 실패");
        return false;
    }
    
    // I2S 초기화
    if (!initialize_i2s()) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "I2S 초기화 실패");
        xSemaphoreGive(i2sMutex);
        return false;
    }
    
    // SPIFFS 뮤텍스 획득
    if (xSemaphoreTake(spiffsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "SPIFFS 뮤텍스 획득 실패");
        xSemaphoreGive(i2sMutex);
        return false;
    }
    
    // 파일 경로 구성
    char filepath[64];
    snprintf(filepath, sizeof(filepath), "%s/%s.wav", SPIFFS_MEDIA_PATH, filename);
    
    logMessage("AUDIO", LOG_LEVEL_INFO, "SPIFFS에서 재생: %s", filepath);
    
    // 파일 존재 확인
    if (!SPIFFS.exists(filepath)) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "파일이 존재하지 않음: %s, SD 카드로 폴백", filepath);
        xSemaphoreGive(spiffsMutex);
        xSemaphoreGive(i2sMutex);
        return play_from_sd(filename, repeatMode, repeatCount);
    }
    
    // 파일 열기
    File file = SPIFFS.open(filepath);
    if (!file) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "파일 열기 실패: %s", filepath);
        xSemaphoreGive(spiffsMutex);
        xSemaphoreGive(i2sMutex);
        return false;
    }
    
    // WAV 헤더 건너뛰기
    uint8_t wav_header[WAV_HEADER_SIZE];
    if (file.read(wav_header, WAV_HEADER_SIZE) != WAV_HEADER_SIZE) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "WAV 헤더 읽기 실패: %s", filepath);
        file.close();
        xSemaphoreGive(spiffsMutex);
        xSemaphoreGive(i2sMutex);
        return false;
    }
    
    // 재생 루프 (반복 모드에 따라)
    int current_repeat = 0;
    
    do {
        // 파일 위치 재설정 (반복 재생 시)
        if (current_repeat > 0) {
            file.seek(WAV_HEADER_SIZE, SeekSet);
        }
        
        // 오디오 데이터 재생
        size_t bytesRead = 0;
        size_t bytesWritten = 0;
        
        while (file.available() && !audioState.stopRequested) {
            bytesRead = file.read((uint8_t*)audioBuffer, BUFFER_SIZE);
            if (bytesRead > 0) {
                // 볼륨 적용
                int samples = bytesRead / 2;
                for (int i = 0; i < samples; i++) {
                    audioBuffer[i] = apply_volume(audioBuffer[i]);
                }
                
                // I2S 출력
                i2s_write(I2S_NUM_0, audioBuffer, bytesRead, &bytesWritten, portMAX_DELAY);
                
                // 태스크 양보 (터치 이벤트 처리 가능)
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
        
        // 반복 횟수 증가
        current_repeat++;
        
        // 중지 요청이 있으면 중단
        if (audioState.stopRequested) {
            break;
        }
        
    } while ((repeatMode == REPEAT_INFINITE) || 
             (repeatMode == REPEAT_COUNT && current_repeat < repeatCount));
    
    // 정리
    file.close();
    i2s_zero_dma_buffer(I2S_NUM_0);
    xSemaphoreGive(spiffsMutex);
    xSemaphoreGive(i2sMutex);
    
    return true;
}
/**
 * 오디오 재생 중지
 */
void audio_stop(void) {
    logMessage("AUDIO", LOG_LEVEL_INFO, "오디오 재생 중지");
    
    // 중지 요청 플래그 설정
    audioState.stopRequested = true;
    
    // 재생 태스크가 있으면 종료 대기
    if (audioState.playbackTaskHandle != NULL) {
        // 몇 ms 동안 대기
        for (int i = 0; i < 10; i++) {
            if (!audioState.isPlaying) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    
    // I2S 버퍼 정리
    if (xSemaphoreTake(i2sMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        i2s_zero_dma_buffer(I2S_NUM_0);
        i2s_stop(I2S_NUM_0);
        xSemaphoreGive(i2sMutex);
    }
    
    // 재생 상태 초기화
    audioState.isPlaying = false;
    audioState.currentSound = NONE;

}

/**
 * 오디오 재생 볼륨 설정
 */
void audio_set_volume(uint8_t vol) {
    current_volume = constrain(vol, 1, 10);
    logMessage("AUDIO", LOG_LEVEL_INFO, "볼륨 설정: %d", vol);
    settings.volume_value = vol;
    saveSettings();
}

/**
 * 현재 볼륨 가져오기
 */
uint8_t audio_get_volume(void) {
    return current_volume;
}

/**
 * 특정 사운드 타입 재생
 */
void audio_play_sound(SoundType sound, AudioRepeatMode repeatMode, int repeatCount) {
    const char* filename = NULL;
    
    switch (sound) {
        case WATER:
            filename = "001";
            break;
            
        case URINATION:
            filename = "002";
            break;
            
        default:
            logMessage("AUDIO", LOG_LEVEL_ERROR, "알 수 없는 사운드 타입: %d", sound);
            return;
    }
    
    // 오디오 재생 시작
    audio_start(filename, repeatMode, repeatCount);
}

/**
 * 물소리 재생 함수
 */
void play_water_sound(AudioRepeatMode repeatMode, int repeatCount) {
    logMessage("AUDIO", LOG_LEVEL_INFO, "물소리 재생 (반복 모드: %d, 횟수: %d)", repeatMode, repeatCount);
    audio_play_sound(WATER, repeatMode, repeatCount);
}

/**
 * 배뇨 사운드 재생 함수
 */
void play_urination_sound(AudioRepeatMode repeatMode, int repeatCount) {
    logMessage("AUDIO", LOG_LEVEL_INFO, "배뇨 사운드 재생 (반복 모드: %d, 횟수: %d)", repeatMode, repeatCount);
    audio_play_sound(URINATION, repeatMode, repeatCount);
}

/**
 * 재생 중인 사운드 중지 함수
 */
void stop_sound() {
    audio_stop();
}

/**
 * 현재 재생 중인지 확인
 */
bool is_audio_playing() {
    return audioState.isPlaying;
}

/**
 * 사운드 플레이어 태스크 - FreeRTOS 태스크로 실행
 */
void soundPlayerTask(void* parameter) {
    SoundType requestedSound;
    
    while (1) {
        // 큐에서 사운드 요청 대기
        if (xQueueReceive(soundQueue, &requestedSound, portMAX_DELAY) == pdTRUE) {
            // 현재 재생 중인 오디오 중지
            if (audioState.isPlaying) {
                audio_stop();
                vTaskDelay(pdMS_TO_TICKS(50));  // 정리 시간 제공
            }
            
            // 요청된 사운드 재생
            audio_play_sound(requestedSound, REPEAT_INFINITE, 0);
        }
        
        // 다른 태스크에 양보
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * 샘플에 볼륨 적용
 */
static int16_t apply_volume(int16_t sample) {
    return (int16_t)(sample * current_volume*0.1);
}

/**
 * I2S 드라이버 초기화
 */
static bool initialize_i2s(void) {
    // 이미 설치된 경우 드라이버 제거
    i2s_driver_uninstall(I2S_NUM_0);
    
    // I2S 설정 초기화
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = true
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = 42,
        .ws_io_num = 2,
        .data_out_num = 41,
        .data_in_num = -1
    };
    
    // 우리 설정으로 드라이버 설치
    if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "I2S 드라이버 설치 실패");
        return false;
    }
    
    // 핀 설정
    if (i2s_set_pin(I2S_NUM_0, &pin_config) != ESP_OK) {
        logMessage("AUDIO", LOG_LEVEL_ERROR, "I2S 핀 설정 실패");
        return false;
    }
    
    // DMA 버퍼 지우기
    i2s_zero_dma_buffer(I2S_NUM_0);
    
    return true;
}

/**
 * 미디어 디렉토리가 존재하는지 확인하고 없으면 생성
 */
static bool ensure_media_directory(void) {
    // SPIFFS 모드에서만 사용됨 - 현재 미구현
    return true;
}