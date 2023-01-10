/*
 * This file is part of the source code of the software program
 * Vampire. It is protected by applicable
 * copyright laws.
 *
 * This source code is distributed under the licence found here
 * https://vprover.github.io/license.html
 * and in the source directory
 */
/**
 * @file LiteralSubstitutionTree.hpp
 * Defines class LiteralSubstitutionTree.
 */


#ifndef __LiteralSubstitutionTree__
#define __LiteralSubstitutionTree__

#include "LiteralIndexingStructure.hpp"
#include "SubstitutionTree.hpp"

namespace Indexing {

/** A wrapper class around SubstitutionTree that makes it usable  as a LiteralIndexingStructure */
class LiteralSubstitutionTree
: public LiteralIndexingStructure
{
  using UnificationsIterator = SubstitutionTree::UnificationsIterator;
  using FastInstancesIterator = SubstitutionTree::FastInstancesIterator;
  using BindingMap = SubstitutionTree::BindingMap;
  using LDIterator = SubstitutionTree::LDIterator;
  using FastGeneralizationsIterator = SubstitutionTree::FastGeneralizationsIterator;
  using QueryResult = SubstitutionTree::QueryResult;
  using LeafData = SubstitutionTree::LeafData;
  using LeafIterator = SubstitutionTree::LeafIterator;
  using Leaf = SubstitutionTree::Leaf;

public:
  CLASS_NAME(LiteralSubstitutionTree);
  USE_ALLOCATOR(LiteralSubstitutionTree);

  LiteralSubstitutionTree(bool useC=false);

  void insert(Literal* lit, Clause* cls);
  void remove(Literal* lit, Clause* cls);
  void handleLiteral(Literal* lit, Clause* cls, bool insert);

  SLQueryResultIterator getAll();

  SLQueryResultIterator getUnifications(Literal* lit, bool complementary, bool retrieveSubstitutions);

  SLQueryResultIterator getUnificationsWithConstraints(Literal* lit, bool complementary, bool retrieveSubstitutions);

  SLQueryResultIterator getGeneralizations(Literal* lit, bool complementary, bool retrieveSubstitutions);

  SLQueryResultIterator getInstances(Literal* lit, bool complementary, bool retrieveSubstitutions);

  SLQueryResultIterator getVariants(Literal* lit, bool complementary, bool retrieveSubstitutions);
  SubstitutionTree& getTree(Literal* lit, bool complementary);

#if VDEBUG
  virtual void markTagged(){ }
#endif

private:
  template<class Iterator>
  SLQueryResultIterator getResultIterator(Literal* lit, bool complementary, bool retrieveSubstitutions, bool useConstraints);

  Stack<unique_ptr<SubstitutionTree>> _trees;
  bool _useC;
};

};

#endif /* __LiteralSubstitutionTree__ */
