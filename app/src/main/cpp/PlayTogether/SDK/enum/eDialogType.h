//
// Created by HMGTEAM on 12/05/2025.
//

#ifndef PLAY_PRO_MAX_EDIALOGTYPE_H
#define PLAY_PRO_MAX_EDIALOGTYPE_H


enum class eDialogType : int {
    None = 0,                       // Không có gì
    BaseResolution = 1,             // Hộp thoại độ phân giải
    BaseLock = 2,                   // Khóa cơ bản
    BaseNotice = 3,                 // Thông báo cơ bản
    BaseTicker = 4,                 // Chữ chạy ticker
    BaseSystem = 5,                 // Hệ thống cơ bản

    BoxMessage = 10,                // Thông báo
    BoxQuestion = 11,               // Câu hỏi (Yes/No)
    BoxInput = 12,                  // Nhập dữ liệu
    BoxLoading = 13,                // Màn hình tải
    ServerCheck = 14,               // Kiểm tra server
    BoxToast = 15,                  // Thông báo nhỏ (toast)
    ServerSidecar = 16,             // Sidecar server
    BoxQuestionIconBtn = 17,        // Câu hỏi kèm nút icon
    ItemRepair = 18,                // Sửa vật phẩm
    TopRequest = 19,                // Yêu cầu nổi bật
    BoxTimeToast = 20,              // Toast có thời gian
    RewardPopup = 21,               // Popup phần thưởng
    MissionCompleted = 22,          // Nhiệm vụ hoàn thành
    BoxSelectCount = 23,            // Chọn số lượng
    BoxFadeEffect = 24,             // Hiệu ứng mờ
    MessageQueueToast = 25,           // Toast theo hàng đợi
    BoxImageTag = 26,               // Hộp gắn ảnh
    UseInstantItem = 27,            // Dùng item ngay
    LinkShare = 28,                 // Chia sẻ link
    PermissionNotice = 29,            // Thông báo quyền truy cập

    LoginMenu = 30,                 // Menu đăng nhập
    ServerSelect = 31,              // Chọn server
    CreateNickname = 32,            // Tạo nickname
    TitleRequestPermission = 33,    // Xin quyền
    EULA = 34,                      // Thỏa thuận EULA
    AccountSelect = 35,             // Chọn tài khoản
    FirstGuide = 36,                // Hướng dẫn đầu tiên
    ServerWaitingLine = 37,         // Xếp hàng server
    RefundAbusingList = 38,         // Danh sách refund abuse
    LoginInput = 39,                // Nhập tài khoản
    SelectHaeginID = 40,            // Chọn Haegin ID
    VersionUpdate = 41,             // Cập nhật phiên bản

    MainMenu = 50,                  // Menu chính

    ShopMain = 60,                  // Shop chính
    ShopInterior = 61,              // Shop nội thất
    ItemRecycle = 62,               // Tái chế item
    ShopBuySelectRight = 63,        // Chọn mua (bên phải)
    ShopBuySelectCenter = 64,       // Chọn mua (giữa)
    ShopBuySelectList = 65,         // Danh sách mua
    ShopInstance = 66,              // Shop instance
    ShopCostume = 67,               // Shop trang phục
    SellShop = 68,                  // Shop bán
    ShopProbTable = 69,             // Bảng xác suất
    ShopHair = 70,                  // Shop tóc
    ShopSocial = 71,                // Shop xã hội
    ShopPartnership = 72,           // Shop hợp tác
    ItemExchange = 73,              // Đổi item
    SelectItemDetail = 74,            // Chi tiết vật phẩm được chọn

    GachaInfoPopup = 80,            // Popup gacha
    PackageInfoPopup = 81,          // Thông tin gói
    EventInfoPopup = 82,            // Thông tin sự kiện
    PackagePreview = 83,            // Xem trước gói
    MultiBuyPopup = 84,             // Mua nhiều lần
    ChoicePackageReceive = 85,      // Nhận gói lựa chọn
    GetItemRoulette = 86,           // Nhận item roulette

    Achievement = 90,               // Thành tích
    DailyMission = 91,              // Nhiệm vụ hàng ngày
    AchievementLvUp = 92,           // Thành tích lên cấp
    TodayADReward = 93,             // Thưởng xem quảng cáo
    DailyStamp = 94,                // Dấu ngày
    BeginnerStamp = 95,             // Dấu tân thủ
    ResultGetItemView = 96,         // Xem item nhận
    ResultGetItemList = 97,         // Danh sách item nhận
    BeginnerGuide = 99,             // Hướng dẫn tân thủ

    PhoneMenu = 100,                // Menu điện thoại
    JoyStick = 101,                 // Joystick
    GameList = 102,                 // Danh sách game
    ChattingWindow = 103,           // Chat
    Costume = 104,                  // Trang phục
    MyBag = 106,                    // Túi đồ
    PlayerMatching = 107,           // Ghép trận
    ActionButtons = 108,            // Nút hành động
    TrophyRoad = 109,               // Con đường danh hiệu
    Desire = 110,                   // Mong muốn
    Cooking = 111,                  // Nấu ăn
    PetInfo = 112,                  // Thông tin thú cưng
    PetCompose = 113,               // Hợp thú cưng
    WorldMap = 114,                 // Bản đồ thế giới
    MailBox = 115,                  // Hộp thư
    GroupPlay = 117,                // Chơi nhóm
    GroupPlayGameList = 118,        // DS game nhóm
    DirectMessage = 119,            // Tin nhắn riêng
    TravelMap = 120,                // Bản đồ du lịch
    PlayTV = 121,                   // TV
    SelfCamera = 122,               // Camera tự sướng
    CageBag = 125,                  // Túi chuồng
    VehicleAccess = 126,            // Truy cập xe
    DirectMessageRead = 127,        // Đọc DM
    UserPermission = 128,             // Quyền người dùng

    HomeGroupPlay = 133,            // Chơi nhóm tại nhà
    MapSelect = 134,                // Chọn bản đồ
    ElevatorConfirm = 135,          // Xác nhận thang máy
    SeasonPassStartView = 136,      // Xem bắt đầu Season Pass
    SeasonPassSelect = 137,         // Chọn Season Pass
    ItemRequestMail = 138,          // Mail yêu cầu item

    BankSaving = 140,               // Ngân hàng tiết kiệm
    BankMinigame = 141,             // Mini game ngân hàng
    FortuneTarot = 142,             // Bói tarot
    FortuneDaily = 143,             // Bói hằng ngày
    MovieList = 144,                // DS phim
    MovieTimeList = 145,            // DS lịch chiếu
    CardMatching = 146,             // Mini game lật bài

    MyHomeMenu = 150,               // Menu nhà
    MyHomeEditInside = 151,         // Sửa bên trong
    MyHomeEditObject = 152,         // Sửa đối tượng
    MyHomeEditOutside = 153,        // Sửa bên ngoài
    MyHomeCollectProp = 154,        // Thu thập vật phẩm
    MyHomeGuide = 155,              // Hướng dẫn nhà
    MyHomeDashBoard = 157,            // Bảng điều khiển nhà
    MyHomeBluePrint = 158,          // Bản thiết kế
    HomeGameList = 159,             // DS game trong nhà
    HomeInfo = 160,                 // Thông tin nhà

    MannequinMenu = 175,            // Menu mannequin
    AINpcCostume = 176,             // NPC AI trang phục
    AINpcCustom = 177,              // NPC AI tùy chỉnh

    HomePartyList = 180,            // DS tiệc tại nhà
    HomePartyCreate = 182,          // Tạo tiệc tại nhà
    HomePartyResult = 183,          // Kết quả tiệc
    HomePartyInfo = 184,            // Thông tin tiệc

    MinigameSimpleInfo = 200,       // Thông tin minigame đơn giản
    CoupleInfo = 201,               // Thông tin couple
    CoupleFlow = 202,               // Dòng sự kiện couple
    CoupleLvUp = 203,               // Couple lên cấp

    PlayerSimpleInfo = 250,         // Thông tin người chơi đơn giản
    FriendList = 251,               // DS bạn bè
    PlayerDetailInfo = 252,         // Thông tin chi tiết người chơi
    FriendInviteReward = 253,       // Thưởng mời bạn
    FriendInviteList = 254,         // DS mời bạn
    ProposalFriendList = 255,       // DS gợi ý bạn bè

    CostumeStyler = 301,            // Styler trang phục

    FishingGetItem = 400,           // Nhận item câu cá
    FishingBait = 401,              // Mồi câu cá
    FishingInfo = 402,              // Thông tin câu cá
    FishingRank = 403,              // BXH câu cá

    ZoneTitle = 410,                // Tiêu đề khu vực

    ItemDetailInfo = 500,           // Chi tiết item
    CompactShop = 501,              // Shop gọn
    CompactSellShop = 502,          // Shop bán gọn

    GuideBook = 600,                // Sổ hướng dẫn

    GameGuide = 1000,               // Hướng dẫn game
    GameCount = 1001,               // Đếm game
    GameResult = 1002,              // Kết quả game
    MiniGame = 1004,                // Mini game
    MiniGameMenu = 1005,            // Menu mini game
    ShopInGame = 1006,              // Shop trong game
    PopupGuide = 1007,              // Popup hướng dẫn
    KMGMain = 1010,                 // KMG chính
    KMGResult = 1011,               // Kết quả KMG

    ArcadeMain = 1020,              // Arcade chính
    ArcadePausePopup = 1021,        // Tạm dừng Arcade
    ArcadeResult = 1022,            // Kết quả Arcade
    ArcadeRevivalPopup = 1023,      // Hồi sinh Arcade

    SimpleGameMain = 1050,          // Game đơn giản chính
    SimpleGameFlow = 1051,          // Luồng game đơn giản

    TreasureHuntRewardList = 1100,  // Thưởng săn kho báu
    GamePass = 1110,                // Game Pass

    AutoCatch = 1200,               // Tự động bắt
    AutoCatchInfo = 1201,           // Thông tin auto catch
    AutoCatchManage = 1202,         // Quản lý auto catch

    Event = 2000,                   // Sự kiện
    EventTalkBox = 2001,            // Hộp thoại sự kiện
    EventTalkBoxYesNo = 2002,       // Hộp thoại sự kiện YesNo
    EventTalkBoxPortrait = 2004,    // Hộp thoại có chân dung
    EventTalkBoxYesNoPortrait = 2005,// Hộp thoại chân dung YesNo
    EventHome = 2007,               // Trang chủ sự kiện
    EventLeague = 2008,             // Giải đấu sự kiện
    EventFlow = 2009,               // Luồng sự kiện
    EventNotice = 2010,             // Thông báo sự kiện
    SelectItemGrid = 2011,          // Chọn item dạng lưới
    EventLeagueInfoPopup = 2012,    // Thông tin giải đấu
    EventPhotoContestVote = 2013,   // Bình chọn ảnh
    EventPhotoContest = 2014,       // Cuộc thi ảnh
    CardCollectMain = 2015,         // Bộ sưu tập thẻ
    CardCollectInfo = 2016,         // Thông tin thẻ
    CardCollectExchange = 2017,     // Đổi thẻ
    BoardMiniGame = 2018,           // Mini game bàn cờ
    TalkBox = 2025,                   // Hộp hội thoại

    QuestProgress = 2101,           // Tiến độ quest
    QuestPopup = 2102,              // Popup quest
    QuestCollectPopup = 2103,       // Thu thập quest

    IllustBook = 2150,              // Sách minh họa
    IllustCollectionInfo = 2151,    // Bộ sưu tập minh họa

    MissionGuidePopup = 2300,       // Popup hướng dẫn mission

    SchoolMain = 2500,              // Trường học chính
    SchoolResult = 2501,            // Kết quả lớp học
    SchoolClassControl = 2502,      // Quản lý lớp học
    SchoolReport = 2503,            // Báo cáo học tập
    SchoolSchedule = 2504,          // Lịch học

    ItemCraftingList = 2600,        // DS chế tạo
    ItemCraftingDetail = 2601,      // Chi tiết chế tạo
    InstantFinishPopup = 2602,      // Hoàn thành ngay
    ShowroomCraft = 2603,           // Chế tạo showroom
    Showroom = 2604,                // Showroom
    ShowroomMain = 2605,            // Showroom chính
    SelectCraftingItemList = 2606,  // DS item chế tạo

    CircleList = 3000,              // DS circle
    CircleMain = 3001,              // Circle chính
    UserCircleSelect = 3002,        // Chọn circle user

    GrowMenu = 5000,                // Menu phát triển
    PetNameEdit = 5001,             // Đổi tên pet
    PetUnlockRiding = 5002,         // Mở cưỡi pet
    PetItemShop = 5003,             // Shop item pet
    PetRecycle = 5004,              // Tái chế pet

    SceneTitle = 10000,             // Cảnh tiêu đề
    CinematicIntro = 10001,         // Mở đầu cinematic
    ActionBase = 10002,             // Hành động cơ bản
    SceneKurrr = 10100,             // Cảnh Kurrr
    SceneMemo = 10101,              // Cảnh Memo

    NoticeScroll = 10201,           // Thông báo cuộn
    GetItemToast = 10202,           // Toast nhận item

    GameOption = 10300,             // Tùy chọn game
    ProductionCrewCredit = 10301,   // Credit sản xuất

    ReviewPopup = 10400,            // Popup đánh giá

    VillageSelect = 10500,          // Chọn làng

    WebView = 10600,                // WebView
    PopupUserReport = 10601,        // Report user
    WebView_Wide = 10602,           // WebView rộng
    ChatUserReport = 10603,           // Báo cáo người dùng trong chat

    TitleLogo = 20000,              // Logo tiêu đề
    RegionLogo = 20001,             // Logo khu vực
    Passport_Cover = 20002,         // Trang bìa hộ chiếu
    Passport_info = 20003,          // Thông tin hộ chiếu

    Chalkboard = 20100,             // Bảng phấn
    PlayReward = 20110,             // Thưởng chơi
    MusicBox = 20120,               // Hộp nhạc
    YoutubeScreen = 20121,          // Màn hình Youtube
    AudioVisualizerScreen = 20122,    // Màn hình hiển thị sóng âm

    RolePlay = 20140,               // Roleplay
    RolePlayGetTag = 20141,         // Nhận tag roleplay
    RolePlaySelect = 20142,         // Chọn roleplay

    BadgeSelect = 20150,            // Chọn huy hiệu

    AccountTransfer = 20200,        // Chuyển tài khoản
    VNGLogo = 20201,                // Logo VNG
    TransferQuestion = 20202,       // Hỏi khi chuyển
    AFK = 20203,                    // AFK

    BabyBag = 20300,                // Túi em bé
    ActorSelector = 20301,          // Chọn nhân vật
    BabyAlbum = 20303,              // Album em bé
    BabyAdopt = 20305,              // Nhận nuôi bé
    BabyItemShop = 20307,           // Shop đồ em bé
    BabyGuide = 20308,              // Hướng dẫn bé
    BabyName = 20309,               // Đặt tên bé
    BabyGraduate = 20312,           // Bé trưởng thành
    BabySelect = 20313,             // Chọn bé
    BabyGrow = 20314,               // Nuôi bé lớn

    PhotoBooth = 20400,             // Phòng chụp ảnh
    PhotoBoothGuide = 20401,        // Hướng dẫn phòng chụp

    MyFarmMenu = 20500,             // Menu nông trại
    MyFarmTimedShop = 20501,        // Shop giới hạn thời gian
    MyFarmTradeList = 20502,        // Danh sách giao dịch
    MyFarmTimedGroupShop = 20503,     // Shop nhóm giới hạn thời gian
    MyFarmCropEdit = 20504,           // Chỉnh sửa cây trồng
    MyFarmCropEditObject = 20505,     // Chỉnh sửa đối tượng cây trồng
    MyFarmGear = 20506,               // Trang bị nông trại
    MyFarmWeatherShop = 20507,        // Shop thời tiết nông trại
    MyFarmStorage = 20508,            // Kho nông trại
    MyFarmFilter = 20509,             // Bộ lọc nông trại
    MyFarmHarvest = 20510,            // Thu hoạch nông trại
    MyFarmPetManage = 20511,          // Quản lý thú cưng nông trại
    BoxButtonList = 20600,            // Danh sách nút dạng hộp

    SnsBoxPurchase = 30003,         // Mua box SNS
    SnsBottomPopup = 30004,         // Popup dưới SNS

    SnsMain = 30101,                // Màn hình SNS
    SnsHomeInfo = 30102,            // Thông tin home SNS
    SnsMyInfoEdit = 30103,          // Sửa thông tin cá nhân
    SnsCostume = 30104,             // Trang phục SNS
    SnsHomePartyCreate = 30105,     // Tạo party tại SNS home
    SnsRequestFriend = 30106,       // Yêu cầu kết bạn SNS
    SnsRecommendedFriend = 30107,   // Bạn bè được gợi ý
    SnsPlayerDetailInfo = 30108,    // Thông tin chi tiết người chơi SNS
    SnsBlockUser = 30109,           // Chặn người dùng SNS
    SnsDiary = 30110,               // Nhật ký SNS
    SnsDiaryWrite = 30111,          // Viết nhật ký SNS
    SnsDiaryFeedback = 30112,       // Phản hồi nhật ký SNS

    SnsMailBox = 30120,             // Hộp thư SNS

    SnsMyFriendCostumeInfo = 30200, // Thông tin trang phục bạn bè
    SnsMyHomeMenu = 30300,          // Menu nhà SNS

    Max = 30301                     // Giá trị tối đa (kết thúc enum)
};


#endif //PLAY_PRO_MAX_EDIALOGTYPE_H
