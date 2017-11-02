#ifndef MP4_PARSER_HH
#define MP4_PARSER_HH

#include <string>
#include <memory>
#include <cstdint>
#include <set>

#include "mp4_file.hh"
#include "box.hh"

namespace MP4 {

const std::set<std::string> mp4_container_boxes{
  "moov", "trak", "edts", "mdia", "minf", "stbl", "mvex", "moof", "traf",
  "mfra", "skip", "strk", "meta", "dinf", "ipro", "sinf", "fiin", "paen",
  "meco", "mere"};

class MP4Parser
{
public:
  /* parse 'filename' as a "reader" by default
   * or write to 'filename' as a "writer" when 'writer' is true */
  MP4Parser(const std::string & filename, const bool writer = false);

  /*
   * functions primarily for MP4 reader
   */
  void parse(); /* parse MP4 into boxes */

  std::shared_ptr<Box> find_first_box_of(const std::string & type);

  /* print the type and size of each box, and the box structure of MP4 */
  void print_structure();

  /*
   * functions primarily for MP4 writer
   */
  void add_top_level_box(std::shared_ptr<Box> && top_level_box);

  void save_mp4_and_close();

  /* TODO: split fragmented MP4 into initial segment and media segments */
  void split(const std::string & init_seg,
             const std::string & media_seg_template,
             const unsigned int start_number);


protected:
  /* accessors */
  std::shared_ptr<MP4File> mp4() { return mp4_; }
  std::shared_ptr<Box> root_box() { return root_box_; }

private:
  std::shared_ptr<MP4File> mp4_;
  std::shared_ptr<Box> root_box_;

  /* a factory method to create different boxes based on their type */
  std::shared_ptr<Box> box_factory(const uint64_t size,
                                   const std::string & type,
                                   const uint64_t data_size);

  /* recursively create boxes between 'start_offset' and its following
   * 'total_size' bytes; add created boxes as children of the 'parent_box' */
  void create_boxes(const std::shared_ptr<Box> & parent_box,
                    const uint64_t start_offset,
                    const uint64_t total_size);

  std::shared_ptr<Box> do_find_first_box_of(const std::shared_ptr<Box> & box,
                                            const std::string & type);

  /* copy size_to_copy bytes from current offset and write to filename */
  void copy_to_file(const std::string & output_filename,
                    const uint64_t size_to_copy);

  std::string populate_template(const std::string & media_seg_template,
                                const unsigned int curr_seg_number);
};

}

#endif /* MP4_PARSER_HH */
