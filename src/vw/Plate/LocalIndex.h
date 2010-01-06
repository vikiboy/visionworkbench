// __BEGIN_LICENSE__
// Copyright (C) 2006-2009 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__


#ifndef __VW_PLATEFILE_LOCAL_INDEX_H__
#define __VW_PLATEFILE_LOCAL_INDEX_H__

#include <vw/Core/FundamentalTypes.h>
#include <vw/Core/Exception.h>
#include <vw/Core/Thread.h>
#include <vw/Core/Log.h>

#include <vw/Image/PixelTypeInfo.h>

#include <vw/Plate/Tree.h>
#include <vw/Plate/Blob.h>
#include <vw/Plate/BlobManager.h>
#include <vw/Plate/RemoteIndex.h>

// Protocol Buffer
#include <vw/Plate/ProtoBuffers.pb.h>

#define VW_PLATE_INDEX_VERSION 2

namespace vw {
namespace platefile {

  // -------------------------------------------------------------------
  //                            LOCAL INDEX
  // -------------------------------------------------------------------

  class LocalIndex : public Index { 
  
  protected:
    std::string m_plate_filename;
    IndexHeader m_header;
    boost::shared_ptr<BlobManager> m_blob_manager;
    boost::shared_ptr<vw::LogInstance> m_log;

    void save_index_file() const;
    std::string index_filename() const;
    std::string log_filename() const;
    std::vector<std::string> blob_filenames() const;
    virtual void rebuild_index(std::string plate_filename) = 0;

  public:

    /// Create a new, empty index.
    LocalIndex( std::string plate_filename, IndexHeader new_index_info);

    /// Open an existing index from a file on disk.
    LocalIndex(std::string plate_filename);

    /// Destructor
    virtual ~LocalIndex() {}

    /// Use this to send data to the index's logfile like this:
    ///
    ///   index_instance.log() << "some text for the log...\n";
    ///
    std::ostream& log ();

    virtual IndexHeader index_header() const { return m_header; }

    // /// Save an index out to a file on disk.  This serializes the
    // /// tree.
    // virtual void save(std::string const& filename);

    virtual int version() const { return m_header.version(); }
    
    virtual std::string platefile_name() const { return m_plate_filename; }

    virtual int32 tile_size() const { return m_header.tile_size(); }
    virtual std::string tile_filetype() const { return m_header.tile_filetype(); }

    virtual PixelFormatEnum pixel_format() const { 
      return PixelFormatEnum(m_header.pixel_format()); 
    }

    virtual ChannelTypeEnum channel_type() const {
      return ChannelTypeEnum(m_header.channel_type());
    }

    // Clients are expected to make a transaction request whenever
    // they start a self-contained chunk of mosaicking work.  .
    virtual int32 transaction_request(std::string transaction_description,
                                      std::vector<TileHeader> const& tile_headers);

    // Once a chunk of work is complete, clients can "commit" their
    // work to the mosaic by issuding a transaction_complete method.
    virtual void transaction_complete(int32 transaction_id);

    // If a transaction fails, we may need to clean up the mosaic.  
    virtual void transaction_failed(int32 transaction_id);

    // Return the current location of the transaction cursor.  This
    // will be the last transaction id that refers to a coherent
    // version of the mosaic.
    virtual int32 transaction_cursor();
  };

  class LocalTreeIndex : public LocalIndex { 
    boost::shared_ptr<TreeNode<IndexRecord> > m_root;

    virtual void rebuild_index(std::string plate_filename);

  public:

    /// Create a new, empty index.
    LocalTreeIndex( std::string plate_filename, IndexHeader new_index_info);

    /// Open an existing index from a file on disk.
    LocalTreeIndex(std::string plate_filename);

    /// Destructor
    virtual ~LocalTreeIndex() {}

    // ----------------------- READ/WRITE REQUESTS  ----------------------

    /// Attempt to access a tile in the index.  Throws an
    /// TileNotFoundErr if the tile cannot be found.
    /// 
    /// By default, this call to read will return a tile with the MOST
    /// RECENT transaction_id <= to the transaction_id you specify
    /// here in the function arguments (if a tile exists).  However,
    /// setting exact_transaction_match = true will force the
    /// PlateFile to search for a tile that has the EXACT SAME
    /// transaction_id as the one that you specify.
    ///
    /// A transaction ID of -1 indicates that we should return the
    /// most recent tile, regardless of its transaction id.
    virtual IndexRecord read_request(vw::int32 col, vw::int32 row, 
                                     vw::int32 level, vw::int32 transaction_id,
                                     bool exact_transaction_match = false);


    /// Return multiple index entries that match the specified
    /// transaction id range.  This range is inclusive of the first
    /// entry, but not the last entry: [ begin_transaction_id, end_transaction_id )
    ///
    /// Results are return as a std::pair<int32, IndexRecord>.  The
    /// first value in the pair is the transaction id for that
    /// IndexRecord.
    virtual std::list<std::pair<int32, IndexRecord> > multi_read_request(int col, int row, int level, 
                                                                         int begin_transaction_id, 
                                                                         int end_transaction_id) {
      vw_throw(NoImplErr() << "multi_read_request() is not yet implemented in LocalTreeIndex");
    }
  
    // Writing, pt. 1: Locks a blob and returns the blob id that can
    // be used to write a tile.
    virtual int write_request(vw::int32 size);

    // Writing, pt. 2: Supply information to update the index and
    // unlock the blob id.
    virtual void write_complete(TileHeader const& header, IndexRecord const& record);

    // ---------------------- TRANSACTIONS  ---------------------

    // Clients are expected to make a transaction request whenever
    // they start a self-contained chunk of mosaicking work.  .
    virtual int32 transaction_request(std::string transaction_description,
                                      std::vector<TileHeader> const& tile_headers);

    // If a transaction fails, we may need to clean up the mosaic.  
    virtual void transaction_failed(int32 transaction_id);

    // ----------------------- PROPERTIES  ----------------------

    /// Returns a list of tile headers for any valid tiles that exist
    /// at a the specified level and transaction_id.  The
    /// transaction_id is treated the same as it would be for
    /// read_request() above.  The region specifies a tile range of
    /// interest.
    virtual std::list<TileHeader> valid_tiles(int level, BBox2i const& region,
                                              int start_transaction_id, 
                                              int end_transaction_id) const {
      vw_throw(NoImplErr() << "valid_tiles() is not yet implemented for LocalTreeIndex");
    }

    virtual int32 num_levels() const { return m_root->num_levels(); }

    /// Use only for debugging small trees.
    void print() { m_root->print(); }

    virtual void map(boost::shared_ptr<TreeMapFunc> func) { m_root->map(func); }
  };


}} // namespace vw::plate

#endif // __VW_PLATEFILE_LOCAL_INDEX_H__
