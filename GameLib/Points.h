/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"
#include "BufferAllocator.h"
#include "ElementContainer.h"
#include "ElementIndexRangeIterator.h"
#include "FixedSizeVector.h"
#include "GameParameters.h"
#include "GameTypes.h"
#include "GameWallClock.h"
#include "IGameEventHandler.h"
#include "Material.h"
#include "RenderContext.h"
#include "Vectors.h"

#include <cassert>
#include <chrono>
#include <cstring>
#include <functional>
#include <vector>

namespace Physics
{

class Points : public ElementContainer
{
public:

    using DestroyHandler = std::function<void(ElementIndex)>;

    enum class EphemeralType
    {
        None,
        Debris,
        Sparkle,
        AirBubble,
        Smoke
    };

private:

    /*
     * The state of ephemeral particles.
     */
    union EphemeralState
    {
        struct DebrisState
        {
        };

        struct SparkleState
        {
        };

        struct AirBubbleState
        {
        };

        struct SmokeState
        {
        };

        DebrisState Debris;
        SparkleState Sparkle;
        AirBubbleState AirBubble;
        SmokeState Smoke;

        EphemeralState(DebrisState debris)
            : Debris(debris)
        {}

        EphemeralState(SparkleState sparkle)
            : Sparkle(sparkle)
        {}

        EphemeralState(AirBubbleState airBubble)
            : AirBubble(airBubble)
        {}

        EphemeralState(SmokeState smoke)
            : Smoke(smoke)
        {}
    };

    /*
     * The elements connected to a point.
     */
    struct Network
    {        
        FixedSizeVector<ElementIndex, GameParameters::MaxSpringsPerPoint> ConnectedSprings; 

        FixedSizeVector<ElementIndex, GameParameters::MaxTrianglesPerPoint> ConnectedTriangles;

        Network()
            : ConnectedSprings()
            , ConnectedTriangles()
        {}
    };

public:

    Points(
        ElementCount shipPointCount,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler)
        : ElementContainer(shipPointCount + GameParameters::MaxEphemeralParticles)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsDeletedBuffer(mBufferElementCount, shipPointCount, false)
        // Material
        , mMaterialBuffer(mBufferElementCount, shipPointCount, nullptr)
        , mIsHullBuffer(mBufferElementCount, shipPointCount, false)
        , mIsRopeBuffer(mBufferElementCount, shipPointCount, false)
        // Mechanical dynamics
        , mPositionBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mVelocityBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mForceBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mIntegrationFactorBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mMassBuffer(mBufferElementCount, shipPointCount, 1.0f)
        // Water dynamics
        , mBuoyancyBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mWaterBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mWaterVelocityBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mWaterMomentumBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mIsLeakingBuffer(mBufferElementCount, shipPointCount, false)
        // Electrical dynamics
        , mElectricalElementBuffer(mBufferElementCount, shipPointCount, NoneElementIndex)
        , mLightBuffer(mBufferElementCount, shipPointCount, 0.0f)
        // Ephemeral particles
        , mEphemeralTypeBuffer(mBufferElementCount, shipPointCount, EphemeralType::None)
        , mEphemeralStartTimeBuffer(mBufferElementCount, shipPointCount, GameWallClock::time_point::min())
        , mEphemeralMaxLifetimeBuffer(mBufferElementCount, shipPointCount, std::chrono::milliseconds::zero())
        , mEphemeralStateBuffer(mBufferElementCount, shipPointCount, EphemeralState::DebrisState())
        // Structure
        , mNetworkBuffer(mBufferElementCount, shipPointCount, Network())
        // Connected component
        , mConnectedComponentIdBuffer(mBufferElementCount, shipPointCount, NoneElementIndex)
        , mCurrentConnectedComponentDetectionVisitSequenceNumberBuffer(mBufferElementCount, shipPointCount, NoneElementIndex)
        // Pinning
        , mIsPinnedBuffer(mBufferElementCount, shipPointCount, false)
        // Immutable render attributes
        , mColorBuffer(mBufferElementCount, shipPointCount, vec4f::zero())
        , mTextureCoordinatesBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mShipPointCount(shipPointCount)
        , mEphemeralPointCount(GameParameters::MaxEphemeralParticles)
        , mAllPointCount(shipPointCount + mEphemeralPointCount)
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventHandler))
        , mDestroyHandler()
        , mAreImmutableRenderAttributesUploaded(false)
        , mFloatBufferAllocator(mBufferElementCount)
        , mVec2fBufferAllocator(mBufferElementCount)
        , mFreeEphemeralParticleSearchStartIndex(mShipPointCount)
        , mAreEphemeralParticlesDirty(false)
    {
    }

    Points(Points && other) = default;

    /*
     * Returns an iterator for the non-ephemeral points only.
     */
    auto NonEphemeralPoints() const
    {
        return ElementIndexRangeIterator(0, mShipPointCount);
    }

    /*
     * Returns an iterator for the ephemeral points only.
     */
    auto EphemeralPoints() const
    {
        return ElementIndexRangeIterator(mShipPointCount, mAllPointCount);
    }

    /*
     * Sets a (single) handler that is invoked whenever a point is destroyed.
     *
     * The handler is invoked right before the point is marked as deleted. However,
     * other elements connected to the soon-to-be-deleted point might already have been
     * deleted.
     *
     * The handler is not re-entrant: destroying other points from it is not supported 
     * and leads to undefined behavior.
     *
     * Setting more than one handler is not supported and leads to undefined behavior.
     */
    void RegisterDestroyHandler(DestroyHandler destroyHandler)
    {
        assert(!mDestroyHandler);
        mDestroyHandler = std::move(destroyHandler);
    }

    void Add(
        vec2f const & position,
        Material const * material,
        bool isHull,
        bool isRope,
        ElementIndex electricalElementIndex,
        float buoyancy,
        vec4f const & color,
        vec2f const & textureCoordinates);

    void CreateEphemeralParticleDebris(
        vec2f const & position,
        vec2f const & velocity,
        Material const * material,
        std::chrono::milliseconds maxLifetime,
        ConnectedComponentId connectedComponentId);

    void Destroy(ElementIndex pointElementIndex);

    void UpdateEphemeralParticles(
        GameWallClock::time_point now,
        GameParameters const & gameParameters);    
    

    //
    // Render
    //

    void Upload(
        int shipId,
        Render::RenderContext & renderContext) const;

    void UploadElements(
        int shipId,
        Render::RenderContext & renderContext) const;

    void UploadVectors(
        int shipId,
        Render::RenderContext & renderContext) const;

    void UploadEphemeralParticles(
        int shipId,
        Render::RenderContext & renderContext) const;

public:

    //
    // IsDeleted
    //

    bool IsDeleted(ElementIndex pointElementIndex) const
    {
        return mIsDeletedBuffer[pointElementIndex];
    }

    //
    // Material
    //

    Material const * GetMaterial(ElementIndex pointElementIndex) const
    {
        return mMaterialBuffer[pointElementIndex];
    }

    bool IsHull(ElementIndex pointElementIndex) const
    {
        return mIsHullBuffer[pointElementIndex];
    }

    bool IsRope(ElementIndex pointElementIndex) const
    {
        return mIsRopeBuffer[pointElementIndex];
    }

    //
    // Dynamics
    //

    vec2f const & GetPosition(ElementIndex pointElementIndex) const
    {
        return mPositionBuffer[pointElementIndex];
    }

    vec2f & GetPosition(ElementIndex pointElementIndex) 
    {
        return mPositionBuffer[pointElementIndex];
    }

    float * restrict GetPositionBufferAsFloat()
    {
        return reinterpret_cast<float *>(mPositionBuffer.data());
    }

    vec2f const & GetVelocity(ElementIndex pointElementIndex) const
    {
        return mVelocityBuffer[pointElementIndex];
    }

    vec2f & GetVelocity(ElementIndex pointElementIndex)
    {
        return mVelocityBuffer[pointElementIndex];
    }

    float * restrict GetVelocityBufferAsFloat()
    {
        return reinterpret_cast<float *>(mVelocityBuffer.data());
    }

    vec2f const & GetForce(ElementIndex pointElementIndex) const
    {
        return mForceBuffer[pointElementIndex];
    }

    vec2f & GetForce(ElementIndex pointElementIndex)
    {
        return mForceBuffer[pointElementIndex];
    }

    float * restrict GetForceBufferAsFloat()
    {
        return reinterpret_cast<float *>(mForceBuffer.data());
    }

    vec2f const & GetIntegrationFactor(ElementIndex pointElementIndex) const
    {
        return mIntegrationFactorBuffer[pointElementIndex];
    }

    float * restrict GetIntegrationFactorBufferAsFloat()
    {
        return reinterpret_cast<float *>(mIntegrationFactorBuffer.data());
    }

    float GetMass(ElementIndex pointElementIndex) const
    {
        return mMassBuffer[pointElementIndex];
    }

    void SetMassToMaterialOffset(
        ElementIndex pointElementIndex,
        float offset,
        Springs & springs);

    // Changes the point's dynamics so that it freezes in place
    // and becomes oblivious to forces
    void Freeze(ElementIndex pointElementIndex)
    {
        // Zero-out integration factor and velocity, freezing point
        mIntegrationFactorBuffer[pointElementIndex] = vec2f(0.0f, 0.0f);
        mVelocityBuffer[pointElementIndex] = vec2f(0.0f, 0.0f);
    }

    // Changes the point's dynamics so that the point reacts again to forces
    void Thaw(ElementIndex pointElementIndex)
    {
        // Re-populate its integration factor, thawing point
        mIntegrationFactorBuffer[pointElementIndex] = CalculateIntegrationFactor(mMassBuffer[pointElementIndex]);
    }

    //
    // Water dynamics
    //

    float GetBuoyancy(ElementIndex pointElementIndex) const
    {
        return mBuoyancyBuffer[pointElementIndex];
    }

    float * restrict GetWaterBufferAsFloat()
    {
        return mWaterBuffer.data();
    }

    float GetWater(ElementIndex pointElementIndex) const
    {
        return mWaterBuffer[pointElementIndex];
    }

    void AddWater(
        ElementIndex pointElementIndex,
        float water)
    {
        mWaterBuffer[pointElementIndex] += water;
        assert(mWaterBuffer[pointElementIndex] >= 0.0f);
    }

    std::shared_ptr<Buffer<float>> MakeWaterBufferCopy()
    {
        auto waterBufferCopy = mFloatBufferAllocator.Allocate();
        waterBufferCopy->copy(mWaterBuffer);

        return waterBufferCopy;
    }

    void UpdateWaterBuffer(std::shared_ptr<Buffer<float>> newWaterBuffer)
    {
        mWaterBuffer.copy(*newWaterBuffer);
    }

    vec2f * restrict GetWaterVelocityBufferAsVec2()
    {
        return mWaterVelocityBuffer.data();
    }

    /*
     * Only valid after a call to UpdateWaterMomentaFromVelocities() and when
     * neither water quantities nor velocities have changed.
     */
    vec2f * restrict GetWaterMomentumBufferAsVec2f()
    {
        return mWaterMomentumBuffer.data();
    }

    void UpdateWaterMomentaFromVelocities()
    {
        float * const restrict waterBuffer = mWaterBuffer.data();
        vec2f * const restrict waterVelocityBuffer = mWaterVelocityBuffer.data();
        vec2f * restrict waterMomentumBuffer = mWaterMomentumBuffer.data();

        for (ElementIndex p = 0; p < mBufferElementCount; ++p)
        {
            waterMomentumBuffer[p] =
                waterVelocityBuffer[p]
                * waterBuffer[p];
        }
    }

    void UpdateWaterVelocitiesFromMomenta()
    {
        float * const restrict waterBuffer = mWaterBuffer.data();
        vec2f * restrict waterVelocityBuffer = mWaterVelocityBuffer.data();
        vec2f * const restrict waterMomentumBuffer = mWaterMomentumBuffer.data();
        
        for (ElementIndex p = 0; p < mBufferElementCount; ++p)
        {
            if (waterBuffer[p] != 0.0f)
            {
                waterVelocityBuffer[p] =
                    waterMomentumBuffer[p]
                    / waterBuffer[p];
            }
            else
            {
                // No mass, no velocity
                waterVelocityBuffer[p] = vec2f::zero();
            }
        }
    }

    bool IsLeaking(ElementIndex pointElementIndex) const
    {
        return mIsLeakingBuffer[pointElementIndex];
    }

    void SetLeaking(ElementIndex pointElementIndex)
    {
        mIsLeakingBuffer[pointElementIndex] = true;
    }

    //
    // Electrical dynamics
    //

    ElementIndex GetElectricalElement(ElementIndex pointElementIndex) const
    {
        return mElectricalElementBuffer[pointElementIndex];
    }

    float GetLight(ElementIndex pointElementIndex) const
    {
        return mLightBuffer[pointElementIndex];
    }

    float & GetLight(ElementIndex pointElementIndex)
    {
        return mLightBuffer[pointElementIndex];
    }

    //
    // Ephemeral Particles
    //

    EphemeralType GetEphemeralType(ElementIndex pointElementIndex) const
    {
        return mEphemeralTypeBuffer[pointElementIndex];
    }

    //
    // Network
    //

    auto const & GetConnectedSprings(ElementIndex pointElementIndex) const
    {
        return mNetworkBuffer[pointElementIndex].ConnectedSprings;
    }

    void AddConnectedSpring(
        ElementIndex pointElementIndex,
        ElementIndex springElementIndex)
    {
        mNetworkBuffer[pointElementIndex].ConnectedSprings.push_back(springElementIndex);
    }

    void RemoveConnectedSpring(
        ElementIndex pointElementIndex,
        ElementIndex springElementIndex)
    {
        bool found = mNetworkBuffer[pointElementIndex].ConnectedSprings.erase_first(springElementIndex);

        assert(found);
        (void)found;
    }

    auto const & GetConnectedTriangles(ElementIndex pointElementIndex) const
    {
        return mNetworkBuffer[pointElementIndex].ConnectedTriangles;
    }

    void AddConnectedTriangle(
        ElementIndex pointElementIndex,
        ElementIndex triangleElementIndex)
    {
        mNetworkBuffer[pointElementIndex].ConnectedTriangles.push_back(triangleElementIndex);
    }

    void RemoveConnectedTriangle(
        ElementIndex pointElementIndex,
        ElementIndex triangleElementIndex)
    {
        bool found = mNetworkBuffer[pointElementIndex].ConnectedTriangles.erase_first(triangleElementIndex);

        assert(found);
        (void)found;
    }

    //
    // Pinning
    //

    bool IsPinned(ElementIndex pointElementIndex) const
    {
        return mIsPinnedBuffer[pointElementIndex];
    }

    void Pin(ElementIndex pointElementIndex) 
    {
        assert(false == mIsPinnedBuffer[pointElementIndex]);
    
        mIsPinnedBuffer[pointElementIndex] = true;

        Freeze(pointElementIndex);
    }

    void Unpin(ElementIndex pointElementIndex)
    {
        assert(true == mIsPinnedBuffer[pointElementIndex]);
    
        mIsPinnedBuffer[pointElementIndex] = false;

        Thaw(pointElementIndex);
    }

    //
    // Connected component
    //

    ConnectedComponentId GetConnectedComponentId(ElementIndex pointElementIndex) const
    {
        return mConnectedComponentIdBuffer[pointElementIndex];
    }

    void SetConnectedComponentId(
        ElementIndex pointElementIndex,
        ConnectedComponentId connectedComponentId)
    { 
        mConnectedComponentIdBuffer[pointElementIndex] = connectedComponentId;
    }

    VisitSequenceNumber GetCurrentConnectedComponentDetectionVisitSequenceNumber(ElementIndex pointElementIndex) const
    {
        return mCurrentConnectedComponentDetectionVisitSequenceNumberBuffer[pointElementIndex];
    }

    void SetCurrentConnectedComponentDetectionVisitSequenceNumber(
        ElementIndex pointElementIndex,
        VisitSequenceNumber connectedComponentDetectionVisitSequenceNumber)
    { 
        mCurrentConnectedComponentDetectionVisitSequenceNumberBuffer[pointElementIndex] =
            connectedComponentDetectionVisitSequenceNumber;
    }

    //
    // Temporary buffer
    //

    std::shared_ptr<Buffer<float>> AllocateWorkBufferFloat()
    {
        return mFloatBufferAllocator.Allocate();
    }

    std::shared_ptr<Buffer<vec2f>> AllocateWorkBufferVec2f()
    {
        return mVec2fBufferAllocator.Allocate();
    }

private:

    static vec2f CalculateIntegrationFactor(float mass);

    ElementIndex FindFreeEphemeralParticle(GameWallClock::time_point const & now);

private:

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    // Deletion
    Buffer<bool> mIsDeletedBuffer;

    // Material
    Buffer<Material const *> mMaterialBuffer;
    Buffer<bool> mIsHullBuffer;
    Buffer<bool> mIsRopeBuffer;

    //
    // Dynamics
    //

    Buffer<vec2f> mPositionBuffer;
    Buffer<vec2f> mVelocityBuffer;
    Buffer<vec2f> mForceBuffer;
    Buffer<vec2f> mIntegrationFactorBuffer;
    Buffer<float> mMassBuffer;

    //
    // Water dynamics
    //

    Buffer<float> mBuoyancyBuffer;
    
    // Height of a 1m2 column of water which provides a pressure equivalent to the pressure at
    // this point. Quantity of water is max(water, 1.0)
    Buffer<float> mWaterBuffer;

    // Total velocity of the water at this point
    Buffer<vec2f> mWaterVelocityBuffer;

    // Total momentum of the water at this point
    Buffer<vec2f> mWaterMomentumBuffer;

    Buffer<bool> mIsLeakingBuffer;

    //
    // Electrical dynamics
    //

    // Electrical element, when any
    Buffer<ElementIndex> mElectricalElementBuffer;

    // Total illumination, 0.0->1.0
    Buffer<float> mLightBuffer;

    //
    // Ephemeral Particles
    //

    Buffer<EphemeralType> mEphemeralTypeBuffer;
    Buffer<GameWallClock::time_point> mEphemeralStartTimeBuffer;
    Buffer<std::chrono::milliseconds> mEphemeralMaxLifetimeBuffer;
    Buffer<EphemeralState> mEphemeralStateBuffer;

    //
    // Structure
    //

    Buffer<Network> mNetworkBuffer;

    //
    // Connected component
    //

    Buffer<ConnectedComponentId> mConnectedComponentIdBuffer; 
    Buffer<VisitSequenceNumber> mCurrentConnectedComponentDetectionVisitSequenceNumberBuffer;

    //
    // Pinning
    //

    Buffer<bool> mIsPinnedBuffer;

    //
    // Immutable render attributes
    //

    Buffer<vec4f> mColorBuffer;
    Buffer<vec2f> mTextureCoordinatesBuffer;


    //////////////////////////////////////////////////////////
    // Container
    //////////////////////////////////////////////////////////

    // Count of ship points; these are followed by ephemeral points
    ElementCount const mShipPointCount;

    // Count of ephemeral points
    ElementCount const mEphemeralPointCount;

    // Count of all points
    ElementCount const mAllPointCount;

    World & mParentWorld;
    std::shared_ptr<IGameEventHandler> const mGameEventHandler;

    // The handler registered for point deletions
    DestroyHandler mDestroyHandler;

    // Flag remembering whether or not we've already uploaded
    // the immutable render attributes
    bool mutable mAreImmutableRenderAttributesUploaded;

    // Allocators for work buffers
    BufferAllocator<float> mFloatBufferAllocator;
    BufferAllocator<vec2f> mVec2fBufferAllocator;

    // The index at which to start searching for free ephemeral particles
    // (just an optimization over restarting from zero each time)
    ElementIndex mFreeEphemeralParticleSearchStartIndex;

    // Flag remembering whether the set of ephemeral particles is dirty
    // (i.e. whether there are more or less particles than previously
    // reported to the rendering engine)
    bool mutable mAreEphemeralParticlesDirty;
};

}