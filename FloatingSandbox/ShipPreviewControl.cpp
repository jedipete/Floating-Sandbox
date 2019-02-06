/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewControl.h"

#include <GameCore/Log.h>

#include <wx/rawbmp.h>

wxDEFINE_EVENT(fsEVT_SHIP_FILE_SELECTED, fsShipFileSelectedEvent);
wxDEFINE_EVENT(fsEVT_SHIP_FILE_CHOSEN, fsShipFileChosenEvent);

ShipPreviewControl::ShipPreviewControl(
    wxWindow * parent,
    std::filesystem::path const & shipFilepath,
    int vMargin,
    RgbaImageData const & waitImage,
    RgbaImageData const & errorImage)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition)
    , mImageGenericStaticBitmap(nullptr)
    , mShipFilepath(shipFilepath)
    , mWaitImage(waitImage)
    , mErrorImage(errorImage)
{
    SetBackgroundColour(wxColour("WHITE"));

    mVSizer = new wxBoxSizer(wxVERTICAL);


    //
    // Image panel
    //

    mImagePanel = new wxPanel(
        this,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(ImageWidth, ImageHeight));

    mImagePanel->SetMinSize(wxSize(ImageWidth, ImageHeight));
    mImagePanel->SetMaxSize(wxSize(ImageWidth, ImageHeight));

    mImagePanel->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mImagePanel->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    // Set initial content to "Wait" image
    SetImageContent(mWaitImage);

    // TODO: see if gets v-centered when smaller
    mVSizer->Add(mImagePanel, 1, wxALIGN_CENTER_HORIZONTAL);



    //
    // Description Label
    //

    mDescriptionLabel = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);

    mDescriptionLabel->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mDescriptionLabel->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    mVSizer->Add(mDescriptionLabel, 0, wxEXPAND);



    //
    // Filename Label
    //

    mFilenameLabel = new wxStaticText(this, wxID_ANY, shipFilepath.filename().string(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);

    mFilenameLabel->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mFilenameLabel->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    mVSizer->Add(mFilenameLabel, 0, wxEXPAND);


    //
    // Bottom margin
    //

    mVSizer->AddSpacer(vMargin);



    this->SetSizer(mVSizer);
}

ShipPreviewControl::~ShipPreviewControl()
{
}

void ShipPreviewControl::SetPreviewContent(ShipPreview const & shipPreview)
{
    //
    // Set image
    //

    SetImageContent(shipPreview.PreviewImage);
    mImagePanel->Refresh();


    //
    // Set description label
    //

    std::string descriptionLabelText = shipPreview.Metadata.ShipName;

    if (!!shipPreview.Metadata.YearBuilt)
        descriptionLabelText += " (" + *(shipPreview.Metadata.YearBuilt) + ")";

    if (!!shipPreview.Metadata.Author)
        descriptionLabelText += " by " + *(shipPreview.Metadata.Author);

    mDescriptionLabel->SetLabel(descriptionLabelText);


    // Rearrange
    mVSizer->Layout();
}

void ShipPreviewControl::OnMouseSingleClick(wxMouseEvent & /*event*/)
{
    //
    // Set border
    //

    // TODO: with Matrioska panels


    //
    // Fire our custom event
    //

    auto event = fsShipFileSelectedEvent(
        fsEVT_SHIP_FILE_SELECTED,
        this->GetId(),
        mShipFilepath);

    ProcessWindowEvent(event);
}

void ShipPreviewControl::OnMouseDoubleClick(wxMouseEvent & /*event*/)
{
    //
    // Fire our custom event
    //

    auto event = fsShipFileChosenEvent(
        fsEVT_SHIP_FILE_CHOSEN,
        this->GetId(),
        mShipFilepath);

    ProcessWindowEvent(event);
}

void ShipPreviewControl::SetImageContent(RgbaImageData const & imageData)
{
    //
    // Create bitmap with content
    //

    wxBitmap bitmap(imageData.Size.Width, imageData.Size.Height, 32);

    wxPixelData<wxBitmap, wxAlphaPixelFormat> pixelData(bitmap);
    if (!pixelData)
    {
        throw std::exception("Cannot get native pixel data");
    }

    assert(pixelData.GetWidth() == imageData.Size.Width);
    assert(pixelData.GetHeight() == imageData.Size.Height);

    rgbaColor const * readIt = imageData.Data.get();
    auto writeIt = pixelData.GetPixels();
    for (int y = 0; y < imageData.Size.Height; ++y)
    {
        // Save current iterator
        auto rowStart = writeIt;

        for (int x = 0; x < imageData.Size.Width; ++x, ++readIt, ++writeIt)
        {
            writeIt.Red() = readIt->r;
            writeIt.Green() = readIt->g;
            writeIt.Blue() = readIt->b;
            writeIt.Alpha() = readIt->a;
        }

        // Move write iterator to next row
        writeIt = rowStart;
        writeIt.OffsetY(pixelData, 1);
    }


    //
    // Create new static bitmap
    //

    // Destroy previous static bitmap
    if (nullptr != mImageGenericStaticBitmap)
        mImageGenericStaticBitmap->Destroy();

    mImageGenericStaticBitmap = new wxGenericStaticBitmap(
        mImagePanel,
        wxID_ANY,
        bitmap,
        wxDefaultPosition,
        wxSize(ImageWidth, ImageHeight));

    mImageGenericStaticBitmap->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mImageGenericStaticBitmap->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    // TODO: see if needed
    mImagePanel->Refresh();
}